/*!
 * \file hd_fake_socket.h
 * \brief socket应用协议
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_FAKE_SOCKET_H
#define HD_FAKE_SOCKET_H

#include <arch/hd_thread_api.h>
#include <ls300/hd_connect.h>

#define MSG_MAX_LEN 128

typedef struct fsocket_t
{
	hd_connect_t *connect;
	semaphore_t send_sem;
	semaphore_t recv_sem;

	e_uint8 id; //0...0xFF
	e_uint8 rq_id; //0...0xFF

#ifdef DEBUG_BUF_CRASH
	volatile int buf_filled;
#endif
	e_uint8 buf[MSG_MAX_LEN];

	e_uint8 name[MSG_MAX_LEN];
	e_int32 state;
} fsocket_t;

/* 接口定义 */
#ifdef __cplusplus
extern "C"
{
#endif

e_int32 DEV_EXPORT fsocket_open(fsocket_t *fs, e_uint8 *name,
		e_uint32 session_id, hd_connect_t *connect);
void DEV_EXPORT fsocket_close(fsocket_t *fs);
void DEV_EXPORT fsocket_reset(fsocket_t *fs);

//timeout_usec为0表示不等待,立即发送
e_int32 DEV_EXPORT fsocket_send(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint32 timeout_usec);
e_int32 DEV_EXPORT fsocket_recv_success(fsocket_t *fs, const e_uint8 *success_reply,
		const e_uint32 rlen, e_uint32 timeout_usec);
e_int32 DEV_EXPORT fsocket_recv(fsocket_t *fs, e_uint8 *recv_buf,
		e_uint32 recv_len, e_uint32 timeout_usec);

e_int32 DEV_EXPORT fsocket_request(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *recv_buf, e_uint32 recv_len, e_uint32 timeout_usec);

e_int32 DEV_EXPORT fsocket_command(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *success_reply, e_uint32 rlen, e_uint32 timeout_usec);
e_int32 DEV_EXPORT fsocket_request_success(fsocket_t *fs, e_uint8 *msg,
		e_uint32 mlen, e_uint8 *recv_buf, e_uint32 recv_len,
		const e_uint8 *success_reply, const e_uint32 rlen, e_uint32 timeout_usec);
e_int32 DEV_EXPORT fsocket_request_failed(fsocket_t *fs, e_uint8 *msg,
		e_uint32 mlen, e_uint8 *recv_buf, e_uint32 recv_len,
		const e_uint8 *failed_reply, const e_uint32 rlen, e_uint32 timeout_usec);

#ifdef __cplusplus
}
#endif

#endif /*HD_FAKE_SOCKET_H*/
