/*!
 * \file hd_fake_socket.c
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
#include <ls300/hd_fake_socket.h>
#include <arch/hd_timer_api.h>

//-------------------------------------------------------------
//#ifdef DMSG
//#undef DMSG
//#define DMSG
//#endif

static e_uint32 send_one_msg(fsocket_t *fs, e_uint8* msg, e_uint32 mlen,
		e_uint32 timeout_usec);
static e_uint32 wait_for_reply(fsocket_t *fs, e_uint32 timeout_usec);
static e_uint32 wait_for_reply_forever(fsocket_t *fs);

e_int32 fsocket_open(fsocket_t *fs, e_uint8 *name, e_uint32 session_id,
		hd_connect_t *connect)
{
	e_int32 ret;
	e_assert(fs&&connect, E_ERROR_INVALID_HANDLER);

	memset(fs, 0, sizeof(fsocket_t));

	fs->connect = connect;
	fs->id = session_id;
	ret = semaphore_init(&fs->send_sem, 1); //发送线路只有一条
	e_assert(ret, E_ERROR_INVALID_CALL);
	ret = semaphore_init(&fs->recv_sem, 0); //消息缓存里现在有0条数据，这里缓存大小为1
	e_assert(ret, E_ERROR_INVALID_CALL);
	if (name)
		strncpy(fs->name, name, sizeof(fs->name));

	fs->state = 1;
	return E_OK;
}

void fsocket_close(fsocket_t *fs)
{
	if (fs->state)
	{
		semaphore_destroy(&fs->send_sem);
		semaphore_destroy(&fs->recv_sem);
		fs->state = 0;
	}
}

//非阻塞的请求模式,需要上层自己控制
e_int32 fsocket_send(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint32 timeout_usec)
{
	e_int32 ret;
	e_assert(fs&&fs->state, E_ERROR_INVALID_HANDLER);
	ret = send_one_msg(fs, msg, mlen, timeout_usec);
	e_assert(ret>0, ret);
	//处理完成，提醒可以发下一个请求了,非阻塞的请求模式
	ret = semaphore_post(&fs->send_sem);
	e_assert(ret>0, ret);
	return E_OK;
}

e_int32 fsocket_recv(fsocket_t *fs, e_uint8 *recv_buf, e_uint32 recv_len,
		e_uint32 timeout_usec)
{
	e_int32 ret;
	e_uint8 req_id;
	e_uint8 s_id;
	int req_iid = -1, s_iid = -1;
	e_assert(fs&&fs->state, E_ERROR_INVALID_HANDLER);
	if (timeout_usec <= 0) //没有设置超时,死等
	{
		ret = wait_for_reply_forever(fs);
	}
	else
	{
		ret = wait_for_reply(fs, timeout_usec);
	}
	if (!e_failed(ret))
	{
		//取出消息,和请求号
		recv_len = recv_len >= MSG_MAX_LEN ? MSG_MAX_LEN : recv_len;
		sscanf(fs->buf, "#%02X%02X%[^@]", &s_iid, &req_iid, recv_buf);
		s_id = s_iid & 0xFF;
		req_id = req_iid & 0xFF;
		//TODO:无视过时消息?
		if (req_id != fs->rq_id)
		{
			DMSG((STDOUT, "取到过时消息:ERROR:request id != fs->req_id"));
		}
		return E_OK;
	}
	return ret;
}

e_int32 fsocket_recv_success(fsocket_t *fs, e_uint8 *success_reply,
		e_uint32 rlen, e_uint32 timeout_usec)
{
	e_int32 ret;
	e_uint8 req_id;
	e_uint8 s_id;
	e_uint8 buf[MSG_MAX_LEN];
	int req_iid = -1, s_iid = -1;
	e_assert(fs&&fs->state, E_ERROR_INVALID_HANDLER);
	if (timeout_usec <= 0) //没有设置超时,死等
	{
		ret = wait_for_reply_forever(fs);
	}
	else
	{
		ret = wait_for_reply(fs, timeout_usec);
	}
	if (!e_failed(ret))
	{
		//取出消息,和请求号
		rlen = rlen >= MSG_MAX_LEN ? MSG_MAX_LEN : rlen;
		sscanf(fs->buf, "#%02X%02X%[^@]", &s_iid, &req_iid, buf);
		s_id = s_iid & 0xFF;
		req_id = req_iid & 0xFF;
		//TODO:无视过时消息?
		if (req_id != fs->rq_id)
		{
			DMSG((STDOUT, "取到过时消息:ERROR:request id != fs->req_id"));
		}
		if (!strncmp(buf, success_reply, rlen))
		{
			return E_OK;
		}
	}

	return E_ERROR;
}

e_int32 fsocket_request(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *recv_buf, e_uint32 recv_len, e_uint32 timeout_usec)
{
	e_int32 ret;
	e_uint8 req_id;
	e_uint8 s_id;
	int req_iid = -1, s_iid = -1;

	/* Timeval structs for handling timeouts */
	e_uint32 beg_time, elapsed_time;

	e_assert(fs&&fs->state, E_ERROR_INVALID_HANDLER);

	DMSG((STDOUT, "[%s_%u]FACK SOCKET do a request ...\r\n", fs->name, fs->id));

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();

	//发送请求
	ret = send_one_msg(fs, msg, mlen, timeout_usec);
	if (e_failed(ret))
		goto END;

//等待回复
	DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET  wait for reply...\r\n", fs->name, fs->id));
	elapsed_time = GetTickCount() - beg_time;

	if (timeout_usec <= 0) //没有设置超时,死等
	{
		ret = wait_for_reply_forever(fs);
	}
	else
	{
		ret = wait_for_reply(fs, timeout_usec - elapsed_time);
	}
	if (!e_failed(ret))
	{
		//取出消息,和请求号
		recv_len = recv_len >= MSG_MAX_LEN ? MSG_MAX_LEN : recv_len;
		sscanf(fs->buf, "#%02X%02X%[^@]", &s_iid, &req_iid, recv_buf);
		s_id = s_iid & 0xFF;
		req_id = req_iid & 0xFF;
		//TODO:无视过时消息?
		if (req_id != fs->rq_id)
		{
			DMSG((STDOUT, "取到过时消息:ERROR:request id != fs->req_id"));
		}
	}

	END:
	//处理完成，提醒可以发下一个请求了
	ret = semaphore_post(&fs->send_sem);
	e_assert(ret, E_ERROR_TIME_OUT);

	elapsed_time = GetTickCount() - beg_time;
	DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET  done.use [%u]Ms...\r\n", fs->name, fs->id, elapsed_time / 1000));
	return E_OK;
}

e_int32 fsocket_command(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *success_reply, e_uint32 rlen, e_uint32 timeout_usec)
{
	e_int32 ret;
	e_int8 buf[MSG_MAX_LEN] =
	{ 0 };

	ret = fsocket_request(fs, msg, mlen, buf, MSG_MAX_LEN, timeout_usec);
	e_assert(ret>0, ret);

	if (!strncmp(buf, success_reply, rlen))
	{
		return E_OK;
	}

	return E_ERROR;
}

e_int32 fsocket_request_success(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *recv_buf, e_uint32 recv_len, e_uint8 *success_reply,
		e_uint32 rlen, e_uint32 timeout_usec)
{
	e_int32 ret;

	ret = fsocket_request(fs, msg, mlen, recv_buf, recv_len, timeout_usec);
	e_assert(ret>0, ret);

	if (!strncmp(recv_buf, success_reply, rlen))
	{
		return E_OK;
	}

	return E_ERROR;
}

e_int32 fsocket_request_failed(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *recv_buf, e_uint32 recv_len, e_uint8 *failed_reply,
		e_uint32 rlen, e_uint32 timeout_usec)
{
	e_int32 ret;

	ret = fsocket_request(fs, msg, mlen, recv_buf, recv_len, timeout_usec);
	e_assert(ret>0, ret);

	if (strncmp(recv_buf, failed_reply, rlen))
	{
		return E_OK;
	}

	return E_ERROR;
}

static e_uint32 send_one_msg(fsocket_t *fs, e_uint8* msg, e_uint32 mlen,
		e_uint32 timeout_usec)
{ //返回msg_id
	e_int32 ret;
	/* Timeval structs for handling timeouts */
	e_uint32 beg_time, elapsed_time;
	e_uint8 buf[MSG_MAX_LEN] =
	{ 0 };

	if (mlen + 6 >= MSG_MAX_LEN)
	{ //消息超长了
		DMSG(
				(STDOUT, "[%s_%u]FACK SOCKET send error:msg is too long...\r\n", fs->name, fs->id));
		return E_ERROR;
	}

	DMSG((STDOUT, "[%s_%u]FACK SOCKET send a msg ...\r\n", fs->name, fs->id));

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();
	sprintf((char*) buf, "#%02X%02X%s@", fs->id, ++fs->rq_id, msg);

	//请求发送锁
	DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET acquaire send sem...\r\n", fs->name, fs->id));
	ret = semaphore_timeoutwait(&fs->send_sem, timeout_usec);
	e_assert(ret, E_ERROR_TIME_OUT);

	DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET request try send ...\r\n", fs->name, fs->id));
	while (fs->state)
	{
		elapsed_time = GetTickCount() - beg_time;
		if (elapsed_time >= timeout_usec)
		{
			DMSG(
					(STDOUT, "[%s_%u]FACK SOCKET send timeout...\r\n", fs->name, fs->id));
			return E_ERROR_TIME_OUT;
		}
		ret = sc_select(fs->connect, E_WRITE, elapsed_time);
		//TODO:更细的异常处理
		if (ret <= 0)
			continue;

		//发送数据
		ret = sc_send(fs->connect, buf, strlen(buf));
		//TODO:更细的异常处理
		if (ret <= 0)
			continue;

		DMSG(
				(STDOUT, "[%s_%u]FACK SOCKET send_one_msg done.\r\n", fs->name, fs->id));
		return E_OK;
	}

	return E_ERROR_INVALID_STATUS;
}

static e_uint32 wait_for_reply(fsocket_t *fs, e_uint32 timeout_usec)
{
	e_int32 ret;

	DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET wait for reply ...\r\n", fs->name, fs->id));
	//等待消息信号
	ret = semaphore_timeoutwait(&fs->recv_sem, timeout_usec);
	e_assert(ret, E_ERROR_TIME_OUT);
	//上层在得到应答后，要马上将缓存的消息拷贝出去，否则会被后来的信息覆盖
	return E_OK;
}

static e_uint32 wait_for_reply_forever(fsocket_t *fs)
{
	e_int32 ret;

	DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET wait for reply ...\r\n", fs->name, fs->id));
	//等待消息信号
	ret = semaphore_wait(&fs->recv_sem);
	e_assert(ret, E_ERROR);
	//上层在得到应答后，要马上将缓存的消息拷贝出去，否则会被后来的信息覆盖
	return E_OK;
}

