/*!
 * \file sick_connect.c
 * \brief 定义了到sick扫描仪连接类
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#include <comm/hd_list.h>
#include <ls300/hd_connect.h>
#include <ls300/hd_laser_base.h>
#include <sickld/sickld_base.h>

enum {
	E_CONNECT_SOCKET = 1, E_CONNECT_COM = 2,
};

static char tostring[1024];

//----------------------------------------------------------------------------
e_int32 sc_open_socket(hd_connect_t* sc, char* sick_ip_address,e_uint16 sick_tcp_port) {
	int ret;
	e_assert(sc, E_ERROR_INVALID_HANDLER);
	memset(sc, 0, sizeof(hd_connect_t));

	if (sick_ip_address == NULL) {
		sick_ip_address = DEFAULT_SICK_IP_ADDRESS;
	}
	if (sick_tcp_port == NULL) {
		sick_tcp_port = DEFAULT_SICK_TCP_PORT;
	}

	ret = Socket_Open(&sc->socket, sick_ip_address, sick_tcp_port, E_TCP);
	e_assert(ret>0, ret);
	sc->state = E_OK;
	sc->mask = E_CONNECT_SOCKET;
	return E_OK;
}

e_int32 sc_open_serial(hd_connect_t* sc, char* com_name, e_uint32 baudrate) {
	int ret;
	e_assert(sc, E_ERROR_INVALID_HANDLER);
	memset(sc, 0, sizeof(hd_connect_t));

	if (com_name == NULL) {
		com_name = DEFAULT_COM_PORT;
	}
	if (baudrate == NULL) {
		baudrate = DEFAULT_COM_BAUDRATE;
	}

	ret = Serial_Open(&sc->serial, com_name);
	e_assert(ret>0, ret);
	ret = Serial_Settings(&sc->serial, baudrate, 'N', 8, 1, 0);
	e_assert(ret>0, ret);
	sc->state = E_OK;
	sc->mask = E_CONNECT_COM;
	return E_OK;
}

e_int32 sc_close(hd_connect_t* sc) {
	e_assert(sc&&sc->state, E_ERROR);

	switch (sc->mask) {
	case E_CONNECT_SOCKET:
		Socket_Close(&sc->socket);
		break;
	case E_CONNECT_COM:
		Serial_Close(&sc->serial);
		break;
	default:
		return E_ERROR;
		break;
	}

	memset(sc, 0, sizeof(hd_connect_t));
	return E_OK;
}

e_int32 sc_state(hd_connect_t *sc) {
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);
	return E_OK;
}

//异步通讯
/*timeout,单位usec, 1秒 = 1000000微秒*/
e_int32 sc_select(hd_connect_t *sc, e_int32 type, e_int32 timeout_usec) {
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);

	switch (sc->mask) {
	case E_CONNECT_SOCKET:
		return Socket_Select(sc->socket, type, timeout_usec);
	case E_CONNECT_COM:
		return Serial_Select(&sc->serial, type, timeout_usec);
	default:
		return E_ERROR_INVALID_HANDLER;
	}
}

e_int32 sc_connect(hd_connect_t *sc) {
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);

	switch (sc->mask) {
	case E_CONNECT_SOCKET:
		return Socket_Connect(sc->socket);
		break;
	case E_CONNECT_COM:
		//check com is ready to write simulate connect processing
		return Serial_Select(&sc->serial, E_WRITE, DEFAULT_COM_TIMEOUT);
	default:
		return E_ERROR_INVALID_HANDLER;
	}
}

e_int32 sc_recv(hd_connect_t *sc, e_uint8 *buffer, e_uint32 blen) {
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);

	switch (sc->mask) {
	case E_CONNECT_SOCKET:
		return Socket_Recv(sc->socket, buffer, blen);
	case E_CONNECT_COM:
		return Serial_Read(&sc->serial, buffer, blen);
	default:
		return E_ERROR_INVALID_HANDLER;
	}
}

e_int32 sc_send(hd_connect_t *sc, e_uint8 *buffer, e_uint32 blen) {
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);
	switch (sc->mask) {
	case E_CONNECT_SOCKET:
		return Socket_Send(sc->socket, buffer, blen);
	case E_CONNECT_COM:
		return Serial_Write(&sc->serial, buffer, blen);
	default:
		return E_ERROR_INVALID_HANDLER;
	}
}

/*in us*/
static e_uint32 compute_elapsed_time(const e_uint32 beg_time, e_uint32 end_time) {
	return (end_time - beg_time);
}

e_int32 sc_request(hd_connect_t *sc, e_uint8 *send_buffer, e_uint32 slen,
		e_uint8 *recv_buffer, e_uint32 rlen, e_uint32 timeout_usec) {
	e_int32 ret;
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);
	/* Timeval structs for handling timeouts */
	e_uint32 beg_time, elapsed_time;

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();

	//发送
	ret = sc_select(sc, E_WRITE, timeout_usec);
	e_assert(ret>0, ret);

	elapsed_time = compute_elapsed_time(beg_time, GetTickCount());
	e_assert(elapsed_time < timeout_usec, E_ERROR_TIME_OUT);

	ret = sc_send(sc, send_buffer, slen);
	e_assert(ret>0, ret);

	while (elapsed_time < timeout_usec) {
		//接收
		ret = sc_select(sc, E_READ, timeout_usec - elapsed_time);
		e_assert(ret>0, ret);

		ret = sc_recv(sc, recv_buffer, rlen); //收到了，就不要管超不超时，直接返回结果
		if (ret > 0)
			break;

		elapsed_time = compute_elapsed_time(beg_time, GetTickCount());
	}

	return E_OK;
}

e_int32 sc_request_and_check(hd_connect_t *sc, e_uint8 *send_buffer,
		e_uint32 slen, e_uint8 *recv_buffer, e_uint32 rlen,
		e_uint8 * check_string, e_uint32 timeout_usec) {
	e_int32 ret;
	e_assert(sc&&sc->state, E_ERROR_INVALID_HANDLER);
	/* Timeval structs for handling timeouts */
	e_uint32 beg_time, elapsed_time;

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();

	//发送
	ret = sc_select(sc, E_WRITE, timeout_usec);
	e_assert(ret>0, ret);

	elapsed_time = compute_elapsed_time(beg_time, GetTickCount());
	e_assert(elapsed_time < timeout_usec, E_ERROR_TIME_OUT);

	ret = sc_send(sc, send_buffer, slen);
	e_assert(ret>0, ret);

	while (elapsed_time < timeout_usec) {
		//接收
		ret = sc_select(sc, E_READ, timeout_usec - elapsed_time);
		e_assert(ret>0, ret);

		ret = sc_recv(sc, recv_buffer, rlen); //收到了，就不要管超不超时，直接返回结果
		if (ret > 0) {
			ret = strncmp(recv_buffer, check_string, rlen);
			if (ret == 0)
				break;
		}

		elapsed_time = compute_elapsed_time(beg_time, GetTickCount());
	}

	return E_OK;
}

char*
sc_tostring(hd_connect_t *sc) {
	e_assert(sc&&sc->state, E_ERROR);
	switch (sc->mask) {
	case E_CONNECT_SOCKET:
		sprintf(tostring, "Sokcet Connect:%s:%u", sc->socket->ip_address,
				sc->socket->port);
		break;
	case E_CONNECT_COM:
		sprintf(tostring, "Serial Connect:%s speed:%u Hz", sc->serial.name,
				sc->serial.speed);
		break;
	default:
		return NULL;
	}
	return tostring;
}
