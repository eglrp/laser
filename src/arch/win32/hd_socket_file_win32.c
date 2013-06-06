//----------------------------------------------------------------------------
//  hd_socket_file_android.c
//  author: Joy.you
//  2013-05-08
//  emai:yjcpui@gmail.com
//----------------------------------------------------------------------------

#ifdef MS_WIN32
#include <hd_socket_api.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

static  struct sigaction sa_old;
static  int is_inited = 0;

void
Socket_Init()
{
    int ret;
    if (is_inited++) return;
    WSADATA wsa;
    //初始化套接字DLL
    ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    e_assert(ret == 0);
}

void
Socket_Quit()
{

    if (!is_inited) return;
    is_inited--;
    if (!is_inited)
        //清理套接字占用的资源
        WSACleanup();
    if (!is_inited) return;

}

//  open/close/state socket device
e_int32
Socket_Open(socket_t **socket_ptr, const char *socket_addr, const e_uint32 port, e_int32 type)
{
    int sockfd;
    socket_t *skt = (socket_t *)malloc(sizeof(socket_t));
    e_assert(skt, E_ERROR_BAD_ALLOCATE);
    memset(skt, 0, sizeof(socket_t));

    //保证经过网络初始化
    Socket_Init();

    /*创建服务器端套接字--IPv4协议*/
    switch (type)
    {
    case SOCKET_TCP:
        /*面向连接通信，TCP协议*/
        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        break;
    case SOCKET_UDP:
        /*无连接，UDP协议*/
        sockfd = socket(PF_INET, SOCK_DGRAM, 0);
        break;
    }
    /*check sockfd*/
    e_assert((sockfd >= 0), E_ERROR_IO);

    /*存储附加信息*/
    skt->priv = (void *)sockfd;
    if (socket_addr != NULL)
        memcpy(skt->ip_address, socket_addr, 16);

    skt->port = port;
    skt->type = type;
    skt->state = E_OK;
    (*socket_ptr) = skt;
    return E_OK;
}

void
Socket_Close(socket_t **socket)
{
    int socketfd;
    if (socket && (*socket))
    {
        if ((*socket)->state)
        {
            socketfd = (int)((*socket)->priv);
            if (socketfd > 0) closesocket(socketfd);
        }
        free(*socket);
        (*socket) = NULL;
    }
}


e_int32
Socket_State(socket_t *socket)
{
    e_assert((socket && socket->state), E_ERROR);
    return E_OK;
}


e_int32
Socket_Ioctrl(socket_t *socket, e_int32 type)
{
    int sockfd;
    e_assert((socket && socket->state), E_ERROR);
    sockfd = (int)socket->priv;

    /* Set the new flags */
    switch (type)
    {
    case SOCKET_BLOCK:
        // If iMode!=0, non-blocking mode is enabled.
        ret = 1;
        ret = ioctlsocket(sockfd, FIONBIO, &ret);
        break;
    case SOCKET_NONBLOCK:
        ret = 0;
        ret = ioctlsocket(sockfd, FIONBIO, &ret);
        break;
    default:
        return E_ERROR_INVALID_PARAMETER;
    }

    e_assert((ret != SOCKET_ERROR), E_ERROR_IO);
    return E_OK;
}

//  select/bind/listen/connect/accept socket connection
/*timeout <= 0 表示永不超时*/
e_int32
Socket_Select(socket_t *socket, e_int32 type, e_int32 timeout_usec)
{
    int sockfd, ret;
    fd_set inputs, checkfds;
    struct timeval timeout;

    e_assert((socket && socket->state), E_ERROR);

    sockfd = (int)socket->priv;

    FD_ZERO(&inputs);//用select函数之前先把集合清零
    FD_SET(0, &inputs); //把要检测的句柄——标准输入（0），加入到集合里。
    while (1)
    {
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
        switch (type)
        {
        case SOCKET_READ:
            if (timeout_usec > 0)
            {
                timeout.tv_sec = (long)(timeout_usec / 1000000);
                timeout.tv_usec = (long)(timeout_usec % 1000000);
                ret = select(sockfd + 1, &checkfds, (fd_set *)0, (fd_set *)0, &timeout);
            }
            else
            {
                ret = select(sockfd + 1, &checkfds, (fd_set *)0, (fd_set *)0, NULL);
            }
            break;
        case SOCKET_WRITE:
            if (timeout_usec > 0)
            {
                timeout.tv_sec = (long)(timeout_usec / 1000000);
                timeout.tv_usec = (long)(timeout_usec % 1000000);
                ret = select(sockfd + 1, (fd_set *)0, &checkfds,  (fd_set *)0, &timeout);
            }
            else
            {
                ret = select(sockfd + 1, (fd_set *)0, &checkfds,  (fd_set *)0, NULL);
            }
            break;
        default:
            return E_ERROR_INVALID_PARAMETER;
        }
        switch (ret)
        {
        case 0:
            e_assert(0, E_ERROR_TIME_OUT);
        case -1:
            /*Select异常，直接返回*/
            e_assert(0, E_ERROR_IO);
        default:
            switch (type)
            {
            case SOCKET_READ:
                if (FD_ISSET(sockfd, &checkfds))
                {
                    ioctl(sockfd, FIONREAD, &ret); //取得数据长度
                    e_assert((ret > 0), E_ERROR_IO); //可读数据长度<=0
                    return ret;
                }
                break;
            case SOCKET_WRITE:
                if (FD_ISSET(sockfd, &checkfds))
                    return E_OK;
                break;
            }
            break;
            break;
        }
    }
}


e_int32
Socket_Bind(socket_t *socket)
{
    int sockfd;
    int ret;
    struct sockaddr_in peer_address;

    e_assert((socket && socket->state), E_ERROR);

    sockfd = (int)socket->priv;

    peer_address.sin_family = AF_INET;

    /*监听的IP可以为空*/
    if (strlen(socket->ip_address) == 0)
        peer_address.sin_addr.s_addr = htonl(INADDR_ANY);
    else
    {
        ret = inet_aton(socket->ip_address, &peer_address.sin_addr); // store IP in antelope
        e_assert(ret, E_ERROR_INVALID_ADDRESS);
    }

    peer_address.sin_port = htons(socket->port);
    ret = bind(sockfd, (struct sockaddr *)&peer_address, sizeof(struct sockaddr_in));
    e_assert((ret != SOCKET_ERROR), E_ERROR_IO);
    return E_OK;
}

/*we never check that whether the socket is binded,just return error code E_ERROR_IO*/
e_int32
Socket_Listen(socket_t *socket)
{
    int sockfd;
    int ret;
    e_assert((socket && socket->state), E_ERROR);
    sockfd = (int)socket->priv;
    //进入侦听状态
    ret = listen(sockfd, SOMAXCONN);
    e_assert((ret != SOCKET_ERROR), E_ERROR_IO);
    return E_OK;
}

e_int32
Socket_Connect(socket_t *socket)
{
    int sockfd;
    int ret;
    struct sockaddr_in peer_address;
    e_assert((socket && socket->state), E_ERROR);
    sockfd = (int)socket->priv;
    peer_address.sin_family = AF_INET;
    ret = inet_aton(socket->ip_address, &peer_address.sin_addr); // store IP in antelope
    e_assert(ret, E_ERROR_INVALID_ADDRESS);
    peer_address.sin_port = htons(socket->port);
    ret = connect(sockfd, (struct sockaddr *)&peer_address, sizeof(struct sockaddr));
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
e_int32
Socket_Accept(socket_t *socket, socket_t **socket_c)
{
    int sockfd;
    int ret;
    socklen_t addr_len;
    char *ip_address;

    socket_t *skt;

    /*接收返回的客户端信息*/
    struct sockaddr_in peer_address;
    e_assert((socket && socket->state), E_ERROR);
    sockfd = (int)socket->priv;
    //接受连接
    ret = accept(sockfd, (struct sockaddr *)&peer_address, &addr_len);
    e_assert((ret != SOCKET_ERROR), E_ERROR_IO);

    /*创建客户会话*/
    skt = (socket_t *)malloc(sizeof(socket_t));
    e_assert(skt, E_ERROR_BAD_ALLOCATE);
    memset(skt, 0, sizeof(socket_t));

    /*存储附加信息*/
    skt->priv = (void *)ret;
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
e_int32
Socket_Recv(socket_t *socket, e_uint8 *buffer, e_uint32 blen)
{
    int sockfd;
    int ret;
    e_assert((socket && socket->state), E_ERROR);
    sockfd = (int)socket->priv;
    /*
    不论是客户还是服务器应用程序都用recv函数从TCP连接的另一端接收数据。
    第二个参数指明一个缓冲区，该缓冲区用来存放recv函数接收到的数据；
    第三个参数指明buf的长度；
    第四个参数一般置0。
    这里只描述同步Socket的recv函数的执行流程。当应用程序调用recv函数时，
    recv先等待s的发送缓冲中的数据被协议传送完毕，如果协议在传送s的发送缓
    冲中的数据时出现网络错误，那么recv函数返回SOCKET_ERROR，如果s的发送
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
    ret = recv(sockfd, buffer, blen, 0); //接收信息
    e_assert(ret > 0, E_ERROR_IO);
    buffer[ret] = '\0';
    return ret;
}

e_int32
Socket_Send(socket_t *socket, unsigned char *buffer, unsigned long len)
{
    int sockfd;
    int ret;
    e_assert((socket && socket->state), E_ERROR);
    sockfd = (int)socket->priv;
    /*
    不论是客户还是服务器应用程序都用send函数来向TCP连接的另一端发送数据。
    客户程序一般用send函数向服务器发送请求，而服务器则通常用send函数来向客户程序发送应答。
    该函数的第一个参数指定发送端套接字描述符；
    第二个参数指明一个存放应用程序要发送数据的缓冲区；
    第三个参数指明实际要发送的数据的字节数；
    第四个参数一般置0。
    这里只描述同步Socket的send函数的执行流程。当调用该函数时，send先比较待发送数据的
    长度len和套接字s的发送缓冲的 长度，如果len大于s的发送缓冲区的长度，该函数返回SOCKET_ERROR；
    如果len小于或者等于s的发送缓冲区的长度，那么send先检查协议 是否正在发送s的发送缓冲中的数据，
    如果是就等待协议把数据发送完，如果协议还没有开始发送s的发送缓冲中的数据或者s的发送缓冲中没有数据，
    那么 send就比较s的发送缓冲区的剩余空间和len，如果len大于剩余空间大小send就一直等待协议把s的
    发送缓冲中的数据发送完，如果len小于剩余 空间大小send就仅仅把buf中的数据copy到剩余空间里（
    注意并不是send把s的发送缓冲中的数据传到连接的另一端的，而是协议传的，send仅仅是把buf中的数据
    copy到s的发送缓冲区的剩余空间里）。如果send函数copy数据成功，就返回实际copy的字节数，如果send
    在copy数据时出现错误，那么send就返回SOCKET_ERROR；如果send在等待协议传送数据时网络断开的话，
    那么send函数也返回SOCKET_ERROR。要注意send函数把buf中的数据成功copy到s的发送缓冲的剩余空间
    里后它就返回了，但是此时这些数据并不一定马上被传到连接的另一端。如 果协议在后续的传送过程中出现
    网络错误的话，那么下一个Socket函数就会返回SOCKET_ERROR。（每一个除send外的Socket函数在执
    行的最开始总要先等待套接字的发送缓冲中的数据被协议传送完毕才能继续，如果在等待时出现网络错误，
    那么该Socket函数就返回 SOCKET_ERROR）注意：在Unix系统下，如果send在等待协议传送数据时网络
    断开的话，调用send的进程会接收到一个SIGPIPE信号，进程对该信号的默认处理是进程终止。
    */
    ret = send(sockfd, buffer, len, 0); //发送信息
    e_assert(ret > 0, E_ERROR_IO);
    return ret;
}

#endif  /*MS_WIN32*/
//----------------------------------------------------------------------------
//


