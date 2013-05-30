#ifdef TEST_SOCKET

#include <hd_socket_api.h>
#include <stdio.h>

#define BUFSIZ 1024
void create_server(){
	socket_t *socketfd,*socket_c;
	int ret;
	char buf[BUFSIZ];  //数据传送的缓冲区

	/*初始化网络库*/
	Socket_Init();

	/*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
	ret = Socket_Open(&socketfd,NULL,8000,SOCKET_TYPE_TCP);
	if(e_failed(ret)) goto E_OUT2;
	
 	/*将套接字绑定到服务器的网络地址上*/
	ret = Socket_Bind(socketfd);
	if(e_failed(ret)) goto E_OUT2;

	/*监听连接请求*/
	ret = Socket_Listen(socketfd);
	if(e_failed(ret)) goto E_OUT2;

	/*等待客户端连接请求到达*/
	ret = Socket_Accept(socketfd,&socket_c);
	if(e_failed(ret)) goto E_OUT1;

	printf("accept client %s\n",socket_c->ip_address);
	ret = Socket_Send(socket_c,"Welcome to my server\n",21);//发送欢迎信息
	if(e_failed(ret)) goto E_OUT1;

	/*接收客户端的数据并将其发送给客户端--recv返回接收到的字节数，send返回发送的字节数*/
	while((ret=Socket_Recv(socket_c,buf,BUFSIZ))>0)
	{
		buf[ret]='\0';
		printf("%s\n",buf);
		ret = Socket_Send(socket_c,buf,ret);
		if(e_failed(ret,"发送异常！")) goto E_OUT1;
	}

E_OUT1:
	Socket_Close(&socket_c);
E_OUT2:
	Socket_Close(&socketfd);

	Socket_Quit();
}

void create_client(){
	int ret;
	socket_t *socketfd;
	char buf[BUFSIZ];  //数据传送的缓冲区

	/*初始化网络库*/
	Socket_Init();

	/*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
	ret = Socket_Open(&socketfd,"127.0.0.1",8000,SOCKET_TYPE_TCP);
	if(e_failed(ret)) goto E_OUT;
	
 	/*连接到服务器*/
	ret = Socket_Connect(socketfd);
	if(e_failed(ret)) goto E_OUT;

	printf("connected to server\n");

	ret = Socket_Recv(socketfd,buf,BUFSIZ);//接收服务器端信息
	if(e_failed(ret)) goto E_OUT;

	buf[ret]='\0';
	printf("%s",buf); //打印服务器端信息

	/*循环的发送接收信息并打印接收信息--recv返回接收到的字节数，send返回发送的字节数*/
	while(1)
	{
		printf("Enter string to send:");
		scanf("%s",buf);
		if(!strcmp(buf,"quit")) break;
		ret = Socket_Send(socketfd, buf, strlen(buf));//发送信息
		if(e_failed(ret)) goto E_OUT;
		ret = Socket_Recv(socketfd,buf,BUFSIZ);//接收服务器端信息
		if(e_failed(ret)) goto E_OUT;
		buf[ret]='\0';
		printf("received:%s\n",buf);
	}

	printf("Bye!\r\n");	

E_OUT:
	Socket_Close(&socketfd);
	Socket_Quit();
}

int main(int argc, char *argv[])
{
	printf("useage: %s -c/-s\r\n",argv[0]);
	if(argc>1){
		if(strncmp(argv[1],"-c",2)==0){
				create_client();
		}else if(strncmp(argv[1],"-s",2)==0){
				create_server();	
		}
	}
	return 0;
}
#endif


