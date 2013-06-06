//----------------------------------------------------------------------------
//	hd_socket_api.h
//  Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------

#ifndef _HD_SOCKET_API_H_
#define _HD_SOCKET_API_H_

//----------------------------------------------------------------------------
//

#include <arch/hd_plat_base.h>

typedef struct socket_t{
	void *priv;
	e_uint32 state; //	socket state: open or not
	e_uint32 ready; //	socket state read to do operation
	e_int32 type;
	e_uint8 ip_address[16];
	e_uint32 port;
}socket_t;
//----------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
// init/quit socket service
	void DEV_EXPORT Socket_Init();
	void DEV_EXPORT Socket_Quit();

//	open/close/state socket device
	e_int32 DEV_EXPORT Socket_Open(socket_t **socket, const char* socket_addr, const e_uint32 port,
		e_int32 type);
	void DEV_EXPORT Socket_Close(socket_t **socket);
	e_int32 DEV_EXPORT Socket_State(socket_t *socket);

//	socket ioctrl,one type at a time 
	e_int32 DEV_EXPORT Socket_Ioctrl(socket_t *socket,e_int32 mask);

//	select/bind/listen/connect/accept socket connection
/*timeout,单位usec, 1秒 = 1000000微秒*/
	e_int32 DEV_EXPORT Socket_Select(socket_t *socket,e_int32 type,e_int32 timeout_usec);
	e_int32 DEV_EXPORT Socket_Bind(socket_t *socket);
	e_int32 DEV_EXPORT Socket_Listen(socket_t *socket);
	e_int32 DEV_EXPORT Socket_Connect(socket_t *socket);
	e_int32 DEV_EXPORT Socket_Accept(socket_t *socket,socket_t **socket_c);

//	read/write socket data
	e_int32 DEV_EXPORT Socket_Recv(socket_t *socket, e_uint8 *buffer, e_uint32 blen);
	e_int32 DEV_EXPORT Socket_Send(socket_t *socket,  e_uint8 *buffer,e_uint32 blen);
	e_int32 DEV_EXPORT Socket_Recvfrom(socket_t *socket, e_uint8 *buffer, e_uint32 blen);
	e_int32 DEV_EXPORT Socket_Sendto(socket_t *socket, e_uint8 *buffer, e_uint32 blen);

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#endif	//	_HD_SOCKET_API_H_
//----------------------------------------------------------------------------
// EOF hd_socket_api.h
