//----------------------------------------------------------------------------
//  hd_com_file_android.c
//
//
//  socket api for andorid
//  author:uplusplus
//  2013-05-08
//  yjcpui@gmail.com
//----------------------------------------------------------------------------

#ifdef ANDROID_OS
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <hd_socket_api.h>

#define SOCKET_ERROR -1

static struct sigaction sa_old={0};
static volatile int is_inited = 0;

void Socket_Init() {
	if (is_inited++)
		return;
	/*在linux下写socket的程序的时候，如果尝试send到一个disconnected socket上，
	 就会让底层抛出一个SIGPIPE信号。这个信号的缺省处理方法是退出进程，大多数时候这都
	 不是我们期望的。因此我们需要重载这个信号的处理方法。调用以下代码，即可安全的屏蔽SIGPIPE：*/
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, &sa_old);
}

void Socket_Quit() {
	if (!is_inited)
		return;
	is_inited--;
	if (!is_inited)
		sigaction(SIGPIPE, &sa_old, 0);
}

//  open/close/state socket device
e_int32 Socket_Open(socket_t **socket_ptr, const char *socket_addr,
		const e_uint32 port, e_int32 type) {
	int sockfd;
	socket_t *skt = (socket_t *) malloc(sizeof(socket_t));
	e_assert(skt, E_ERROR_BAD_ALLOCATE);
	memset(skt, 0, sizeof(socket_t));

	//保证经过网络初始化
	Socket_Init();

	/*创建服务器端套接字--IPv4协议*/
	switch (type) {
	case E_TCP:
		/*面向连接通信，TCP协议*/
		sockfd = socket(PF_INET, SOCK_STREAM, 0);
		break;
	case E_UDP:
		/*无连接，UDP协议*/
		sockfd = socket(PF_INET, SOCK_DGRAM, 0);
		break;
	}
	/*check sockfd*/
	if (e_failed(sockfd)) {
		free(skt);
		return E_ERROR_IO;
	}

	/*存储附加信息*/
	skt->priv = (void *) sockfd;
	if (socket_addr != NULL)
		memcpy(skt->ip_address, socket_addr, 16);

	skt->port = port;
	skt->type = type;
	skt->state = E_OK;
	(*socket_ptr) = skt;
	return E_OK;
}

void Socket_Close(socket_t **socket) {
	int socketfd;
	if (socket && (*socket)) {
		if ((*socket)->state) {
			socketfd = (int) ((*socket)->priv);
			if (socketfd > 0)
				close(socketfd);
		}
		free(*socket);
		(*socket) = NULL;
	}
}

e_int32 Socket_State(socket_t *socket) {
	e_assert((socket && socket->state), E_ERROR);
	return E_OK;
}

e_int32 Socket_Ioctrl(socket_t *socket, e_int32 type) {
	int sockfd;
	e_assert((socket && socket->state), E_ERROR);
	sockfd = (int) socket->priv;
	int fd_flags = 0;
	fd_flags = fcntl(sockfd, F_GETFL);
	e_assert((fd_flags >= 0), E_ERROR_IO);

	/* Set the new flags */
	switch (type) {
	case E_BLOCK:
		fd_flags = fcntl(sockfd, F_SETFL, fd_flags & (~O_NONBLOCK));
		break;
	case E_NONBLOCK:
		fd_flags = fcntl(sockfd, F_SETFL, fd_flags | O_NONBLOCK);
		break;
	default:
		return E_ERROR_INVALID_PARAMETER;
	}

	e_assert((fd_flags >= 0), E_ERROR_IO);
	return E_OK;
}

//  select/bind/listen/connect/accept socket connection
/* timeout <= 0 表示永不超时 */
e_int32 Socket_Select(socket_t *socket, e_int32 type, e_int32 timeout_usec) {
	int sockfd, ret;
	fd_set inputs, checkfds;
	struct timeval timeout;

	e_assert((socket && socket->state), E_ERROR);

	sockfd = (int) socket->priv;
	//用select函数之前先把集合清零
	FD_ZERO(&inputs);

	//把要检测的句柄sockfd，加入到集合里。
	FD_SET(sockfd, &inputs);

	while (1) {
		checkfds = inputs;
		/*如果参数timeout设为NULL则表示select（）没有timeout
		 执行成功则返回文件描述词状态已改变的个数，如果返回0代表在描述词状态改变前已超过timeout时间，
		 当有错误发生时则返回-1，错误原因存于errno，此时参数readfds，writefds，exceptfds和
		 timeout的值变成不可预测。
		 EBADF 文件描述词为无效的或该文件已关闭
		 EINTR 此调用被信号所中断
		 EINVAL 参数n 为负值。
		 ENOMEM 核心内存不足
		 */
		switch (type) {
		case E_READ:
			//DMSG(( STDOUT,"Socket_Select FD=%d timeout=%dus",sockfd, timeout_usec));
			if (timeout_usec > 0) {
				timeout.tv_sec = (long) (timeout_usec / 1000000);
				timeout.tv_usec = (long) (timeout_usec % 1000000);
				ret = select(sockfd + 1, &checkfds, (fd_set *) 0, (fd_set *) 0,
						&timeout);
			} else {
				ret = select(sockfd + 1, &checkfds, (fd_set *) 0, (fd_set *) 0,
						NULL);
			}
			//DMSG((STDOUT,"Socket_Select %d socket can read/write",ret));
			break;
		case E_WRITE:
			if (timeout_usec > 0) {
				timeout.tv_sec = (long) (timeout_usec / 1000000);
				timeout.tv_usec = (long) (timeout_usec % 1000000);
				ret = select(sockfd + 1, (fd_set *) 0, &checkfds, (fd_set *) 0,
						&timeout);
			} else {
				ret = select(sockfd + 1, (fd_set *) 0, &checkfds, (fd_set *) 0,
						NULL);
			}
			break;
		default:
			return E_ERROR_INVALID_PARAMETER;
		}

		if (e_check(ret!=0,"Socket select time out.")) {
			return E_ERROR_TIME_OUT;
		}
//		e_assert(ret!=0, E_ERROR_TIME_OUT);
		//超时
		e_assert(ret>0, E_ERROR_IO);
		//ＩＯ异常

		switch (type) {
		case E_READ:
			ret = FD_ISSET(sockfd, &checkfds);
			e_assert(ret, E_ERROR_IO);
			ioctl(sockfd, FIONREAD, &ret); //取得数据长度
			e_assert((ret > 0), E_ERROR_IO);
			//DMSG((STDOUT,"Socket_Select %d data can read",ret));
			//可读数据长度>0
			return ret;
		case E_WRITE:
			ret = FD_ISSET(sockfd, &checkfds);
			e_assert(ret, E_ERROR_IO);
			//DMSG((STDOUT,"Socket_Select socket ready to write"));
			return E_OK;
		}

	}
	return E_ERROR;
}

e_int32 Socket_Bind(socket_t *socket) {
	int sockfd;
	int ret;
	struct sockaddr_in peer_address;

	e_assert((socket && socket->state), E_ERROR);

	sockfd = (int) socket->priv;

	peer_address.sin_family = AF_INET;

	/*监听的IP可以为空*/
	if (strlen(socket->ip_address) == 0)
		peer_address.sin_addr.s_addr = htonl(INADDR_ANY);
	else {
		ret = inet_aton(socket->ip_address, &peer_address.sin_addr); // store IP in antelope
		e_assert(ret, E_ERROR_INVALID_ADDRESS);
	}

	peer_address.sin_port = htons(socket->port);
	ret = bind(sockfd, (struct sockaddr *) &peer_address,
			sizeof(struct sockaddr_in));
	e_assert((ret != SOCKET_ERROR), E_ERROR_IO);
	return E_OK;
}

/*we never check that whether the socket is binded,just return error code E_ERROR_IO*/
e_int32 Socket_Listen(socket_t *socket) {
	int sockfd;
	int ret;
	e_assert((socket && socket->state), E_ERROR);
	sockfd = (int) socket->priv;
	//进入侦听状态
	ret = listen(sockfd, SOMAXCONN);
	e_assert((ret != SOCKET_ERROR), E_ERROR_IO);
	return E_OK;
}

e_int32 Socket_Connect(socket_t *socket) {
	int sockfd;
	int ret;
	struct sockaddr_in peer_address;
	e_assert((socket && socket->state), E_ERROR);
	sockfd = (int) socket->priv;
	peer_address.sin_family = AF_INET;
	ret = inet_aton(socket->ip_address, &peer_address.sin_addr); // store IP in antelope
	e_assert(ret, E_ERROR_INVALID_ADDRESS);
	peer_address.sin_port = htons(socket->port);
	ret = connect(sockfd, (struct sockaddr *) &peer_address,
			sizeof(struct sockaddr));
	e_assert((ret != SOCKET_ERROR), E_ERROR_IO);
	return E_OK;
}

/*
 inet_aton() returns non-zero if the address is a valid one, and it returns zero if the address is invalid.
 inet_ntoa() returns the dots-and-numbers string in a static buffer that is overwritten with each call to
 the function.
 inet_addr() returns the address as an in_addr_t, or -1 if there's an error. (That is the same result
 as if you tried to convert the string "255.255.255.255", which is a valid IP address. This is why inet_aton()
 is better.)
 #include<sys/socket.h>
 int accept(int sockfd, struct sockaddr* addr, socklen_t* len)
 返回：非负描述字——成功， -1——失败
 对于服务器编程中最重要的一步等待并接受客户的连接，那么这一步在编程中如何完成，accept函数就是完成这一步的。
 它从内核中取出已经建立的客户连接，然后把这个已经建立的连接返回给用户程序，此时用户程序就可以与自己的客户进行点到点的通信了。

 */
e_int32 Socket_Accept(socket_t *socket, socket_t **socket_c) {
	int sockfd;
	int ret;
	socklen_t addr_len;
	char *ip_address;

	socket_t *skt;

	/*接收返回的客户端信息*/
	struct sockaddr_in peer_address;
	e_assert((socket && socket->state), E_ERROR);
	sockfd = (int) socket->priv;
	//接受连接
	ret = accept(sockfd, (struct sockaddr *) &peer_address, &addr_len);
	e_assert((ret != SOCKET_ERROR), E_ERROR_IO);

	/*创建客户会话*/
	skt = (socket_t *) malloc(sizeof(socket_t));
	e_assert(skt, E_ERROR_BAD_ALLOCATE);
	memset(skt, 0, sizeof(socket_t));

	/*存储附加信息*/
	skt->priv = (void *) ret;
	ip_address = inet_ntoa(peer_address.sin_addr); // resolve IP in antelope
	e_assert(ip_address, E_ERROR_INVALID_ADDRESS);
	strncpy(skt->ip_address, ip_address, 16);
	skt->port = ntohs(peer_address.sin_port);
	skt->type = socket->type;
	skt->state = E_OK;
	(*socket_c) = skt;
	return E_OK;
}

//  read/write socket data
e_int32 Socket_Recv(socket_t *socket, e_uint8 *buffer, e_uint32 blen) {
	int sockfd;
	int byteRecvied = 0, byteCount = 0;
	e_assert((socket && socket->state), E_ERROR);
	sockfd = (int) socket->priv;
	/*
	 不论是客户还是服务器应用程序都用recv函数从TCP连接的另一端接收数据。
	 第二个参数指明一个缓冲区，该缓冲区用来存放recv函数接收到的数据；
	 第三个参数指明buf的长度；
	 第四个参数一般置0。
	 这里只描述同步Socket的recv函数的执行流程。当应用程序调用recv函数时，
	 recv先等待s的发送缓冲中的数据被协议传送完毕，如果协议在传送s的发送缓
	 冲中的数据时出现网络错误，那么recv函数返回E_ERROR，如果s的发送
	 缓冲中没有数据或者数据被协议成功发送完毕后，recv先检查套接字s的接收缓
	 冲区，如果s接收缓冲区中没有数据或者协议正在接收数据，那么recv就一直等待，
	 只到协议把数据接收完毕。当协议把数据接收完毕，recv函数就把s的接收缓冲
	 中的数据copy到buf中（注意协议接收到的数据可能大于buf的长度，所以 在这
	 种情况下要调用几次recv函数才能把s的接收缓冲中的数据copy完。recv函数仅
	 仅是copy数据，真正的接收数据是协议来完成的），recv函数返回其实际copy的
	 字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；如果recv函数
	 在等待协议接收数据时网络中断了，那么它返回0。
	 注意：在Unix系统下，如果recv函数在等待协议接收数据时网络断开了，那么调用
	 recv的进程会接收到一个SIGPIPE信号，进程对该信号的默认处理是进程终止。
	 */

	//接收信息
	while (byteRecvied != blen) {
		/* get bytes from port */
		byteCount = recv(sockfd, buffer + byteRecvied, blen - byteRecvied, 0);
		byteRecvied += byteCount;

		/* if no bytes read timeout return byteRecvied */
		if (byteCount == 0) {
			break;
		}
	}
	return byteRecvied;
}

e_int32 Socket_Send(socket_t *socket, e_uint8 *buffer, e_uint32 blen) {
	int sockfd;
	int byteSend = 0, byteCount = 0;
	e_assert((socket && socket->state), E_ERROR);
	sockfd = (int) socket->priv;
	/*
	 不论是客户还是服务器应用程序都用send函数来向TCP连接的另一端发送数据。
	 客户程序一般用send函数向服务器发送请求，而服务器则通常用send函数来向客户程序发送应答。
	 该函数的第一个参数指定发送端套接字描述符；
	 第二个参数指明一个存放应用程序要发送数据的缓冲区；
	 第三个参数指明实际要发送的数据的字节数；
	 第四个参数一般置0。
	 这里只描述同步Socket的send函数的执行流程。当调用该函数时，send先比较待发送数据的
	 长度len和套接字s的发送缓冲的 长度，如果len大于s的发送缓冲区的长度，该函数返回E_ERROR；
	 如果len小于或者等于s的发送缓冲区的长度，那么send先检查协议 是否正在发送s的发送缓冲中的数据，
	 如果是就等待协议把数据发送完，如果协议还没有开始发送s的发送缓冲中的数据或者s的发送缓冲中没有数据，
	 那么 send就比较s的发送缓冲区的剩余空间和len，如果len大于剩余空间大小send就一直等待协议把s的
	 发送缓冲中的数据发送完，如果len小于剩余 空间大小send就仅仅把buf中的数据copy到剩余空间里（
	 注意并不是send把s的发送缓冲中的数据传到连接的另一端的，而是协议传的，send仅仅是把buf中的数据
	 copy到s的发送缓冲区的剩余空间里）。如果send函数copy数据成功，就返回实际copy的字节数，如果send
	 在copy数据时出现错误，那么send就返回SOCKET_ERROR；如果send在等待协议传送数据时网络断开的话，
	 那么send函数也返回E_ERROR。要注意send函数把buf中的数据成功copy到s的发送缓冲的剩余空间
	 里后它就返回了，但是此时这些数据并不一定马上被传到连接的另一端。如 果协议在后续的传送过程中出现
	 网络错误的话，那么下一个Socket函数就会返回E_ERROR。（每一个除send外的Socket函数在执
	 行的最开始总要先等待套接字的发送缓冲中的数据被协议传送完毕才能继续，如果在等待时出现网络错误，
	 那么该Socket函数就返回 SOCKET_ERROR）注意：在Unix系统下，如果send在等待协议传送数据时网络
	 断开的话，调用send的进程会接收到一个SIGPIPE信号，进程对该信号的默认处理是进程终止。
	 */
	//发送信息
	while (byteSend != blen) {
		/* get bytes from port */
		byteCount = send(sockfd, buffer + byteSend, blen - byteSend, 0);
		byteSend += byteCount;

		/* if no bytes send return byteSend */
		if (byteCount == 0) {
			break;
		}
	}
	return byteSend;
}

#endif  /*ANDROID_OS*/
//----------------------------------------------------------------------------
//

