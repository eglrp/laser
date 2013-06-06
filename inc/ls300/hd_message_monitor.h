/*!
 * \file hd_message_monitor.h
 * \brief	通讯总控
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd laser base
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_MESSAGE_MONITOR_H
#define HD_MESSAGE_MONITOR_H

#include "hd_connect.h"
#include "hd_fake_socket.h"

#define MAX_CLIENT_SIZE 10
#define MM_SLEEP_TIMEOUT (1000*1000)

typedef struct msg_monitor_t {
	hd_connect_t *connect;
	ethread_t *thread_loop;

	//socket最大数
	fsocket_t sockets[MAX_CLIENT_SIZE];
	e_uint32 sockets_num;

	volatile e_int32 state;
}msg_monitor_t;

/* 接口定义 */
#ifdef __cplusplus
extern "C" {
#endif

e_int32 DEV_EXPORT mm_init(msg_monitor_t *mm, hd_connect_t *sc);
e_int32 DEV_EXPORT mm_start(msg_monitor_t *mm);
e_int32 DEV_EXPORT mm_stop(msg_monitor_t *mm);
//not thread safe
fsocket_t* DEV_EXPORT mm_create_socket(msg_monitor_t *mm, e_uint8* name);

#ifdef __cplusplus
}
#endif

#endif /*HD_MESSAGE_MONITOR_H*/
