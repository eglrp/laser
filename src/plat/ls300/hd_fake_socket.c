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
#ifdef DMSG
#undef DMSG
#define DMSG
#endif

static e_uint32 send_one_msg(fsocket_t *fs, e_uint8* msg, e_uint32 mlen,
		e_uint32 timeout_usec);
static e_uint32 wait_for_reply(fsocket_t *fs, e_uint32 timeout_usec);
static e_uint32 wait_for_reply_forever(fsocket_t *fs);

/**
 *\brief 创建网络会话进程。
 *\param fs 定义了会话进程描述符。
 *\param name 定义了进程标识符。
 *\param session_id 定义了进程描述符的ID。
 *\param connect 定义了海达连接属性。
 *\retval E_OK 表示成功。
 */
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

/**
 *\brief 关闭网络会话进程。
 *\param fs 定义了会话进程描述符。
 *\retval E_OK 表示成功。
 */
void fsocket_close(fsocket_t *fs)
{
	if (fs->state)
	{
		semaphore_destroy(&fs->send_sem);
		semaphore_destroy(&fs->recv_sem);
		fs->state = 0;
	}
}

void fsocket_reset(fsocket_t *fs)
{
	e_int32 ret;
	if (fs->state)
	{
		ret = semaphore_timeoutwait(&fs->recv_sem, 1);
		if (!ret) {
			fs->buf[0] = 0;
			semaphore_post(&fs->recv_sem); //让等待消息的队列清空
		}
	}
}

/**
 *\brief 通过网络发送数据得发送函数，非阻塞的请求模式,需要上层自己控制。
 *\param fs 定义了套接子得相关属性。
 *\param msg 定义了需要发送得消息。
 *\param mlen 定义了需要发送得消息得长度。
 *\param timeout_usec 定义消息发送得超时时间。
 *\retval E_OK 表示成功。
 */
e_int32 fsocket_send(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint32 timeout_usec)
{
	e_int32 ret;
	e_assert(fs&&fs->state, E_ERROR_INVALID_HANDLER);
	ret = send_one_msg(fs, msg, mlen, timeout_usec);
	//处理完成，提醒可以发下一个请求了,非阻塞的请求模式
	if (ret != E_ERROR_INVALID_CALL) //已经获取信号量
		semaphore_post(&fs->send_sem);
	return ret;
}

#if 0
/**
 *\brief 通过网络接收数据得接收函数。
 *\param fs 定义了套接子得相关属性。
 *\param recv_buf 定义了接收消息的缓存。
 *\param recv_len 定义了接收消息的长度。
 *\param timeout_usec 定义消息接收的超时时间。
 *\retval E_OK 表示成功。
 */
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
			DMSG(
					(STDOUT, "fsocket_recv 取到过时消息:ERROR:request id[%u] != fs->req_id[%u]\n",req_id,fs->rq_id));
		}
		return E_OK;
	}
	return ret;
}

/**
 *\brief 通过网络成功接收数据的接收函数。
 *\param fs 定义了套接子得相关属性。
 *\param success_reply 定义了成功接收数据的缓存。
 *\param recv_len 定义了接收消息的长度。
 *\param timeout_usec 定义消息接收的超时时间。
 *\retval E_OK 表示成功。
 */
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
		if (req_id < fs->rq_id)
		{
			DMSG(
					(STDOUT, "fsocket_recv_success 取到过时消息:ERROR:request id[%u] != fs->req_id[%u]\n",req_id,fs->rq_id));
			return E_ERROR;
		}
		else if (req_id > fs->rq_id)
		{
			DMSG((STDOUT,"出现消息号异常,请检查!\n"));
			while (1)
			;
		}
		if (!strncmp(buf, success_reply, rlen))
		{
			return E_OK;
		}
	}

	return E_ERROR;
}

#endif

/**
 *\brief 网络连接发送请求函数。
 *\param fs 定义了套接子得相关属性。
 *\param msg 定义了发送请求消息。
 *\param mlen 定义了发送请求消息长度。
 *\param recv_buf 定义了接收缓存。
 *\param recv_len 定义了接收消息长度。
 *\param timeout_usec 定义了接收消息超时时间。
 *\retval E_OK 表示成功。
 */
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

	DMSG((STDOUT, "[%s_%u]FACK SOCKET do a request ...\r\n", fs->name, (unsigned int) fs->id));

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();

	//发送请求
	ret = send_one_msg(fs, msg, mlen, timeout_usec);
	if (e_failed(ret))
		goto END;

//等待回复
	elapsed_time = GetTickCount() - beg_time;

	while (timeout_usec <= 0
			|| (elapsed_time = GetTickCount() - beg_time) < timeout_usec)
	{
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
			if (req_id < fs->rq_id)
					{
				DMSG(
				(STDOUT, "fsocket_request 取到过时消息:ERROR:request id[%u] != fs->req_id[%u]\n忽略,继续等待下一个消息\n", req_id, fs->rq_id));
				continue;
			}
			else if (req_id > fs->rq_id)
					{
//				DMSG((STDOUT,"出现消息号异常,请检查!\n"));
//				while (1)
//					;
			}
			break;
		}
	}

	END:
	//处理完成，提醒可以发下一个请求了
	ret = semaphore_post(&fs->send_sem);
	e_assert(ret, E_ERROR_TIME_OUT);

	elapsed_time = GetTickCount() - beg_time;
	DMSG((STDOUT, "[%s_%u]FACK SOCKET  done.use [%u]Ms...\r\n", fs->name, (unsigned int) fs->id, (int) (elapsed_time
			/ 1000)));
	return E_OK;
}

/**
 *\brief 发送网络命令。
 *\param fs 定义了套接子得相关属性。
 *\param msg 定义了发送的命令缓存。 
 *\param mlen 定义了发送的命令长度。const
 *\param success_reply 定义了获得返回的信息缓存。
 *\param rlen 定义了返回数据的长度信息。
 *\param timeout_usec 定义了命令请求超时得时间。 
 *\retval E_OK 表示成功。
 */
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

/**
 *\brief 发送网络请求成功。
 *\param fs 定义了套接子得相关属性。
 *\param msg 定义了发送的命令缓存。 
 *\param mlen 定义了发送的命令长度。
 *\param recv_buf 定义了获得返回的信息缓存。
 *\param recv_len 定义了返回数据的长度信息。
 *\param success_reply 定义了获得返回成功的信息缓存。
 *\param rlen 定义了返回成功数据的长度信息。 
 *\param timeout_usec 定义了命令请求超时得时间。 
 *\retval E_OK 表示成功。
 */
e_int32 fsocket_request_success(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *recv_buf, e_uint32 recv_len, const e_uint8 *success_reply,
		const e_uint32 rlen, e_uint32 timeout_usec)
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

/**
 *\brief 发送网络请求,并检测请求结果是否成功失败。
 *\param fs 定义了套接子得相关属性。
 *\param msg 定义了发送的命令缓存。 
 *\param mlen 定义了发送的命令长度。
 *\param recv_buf 定义了获得返回的信息缓存。
 *\param recv_len 定义了返回数据的长度信息。
 *\param success_reply 定义了获得返回失败的信息缓存。
 *\param rlen 定义了返回失败数据的长度信息。 
 *\param timeout_usec 定义了命令请求超时得时间。 
 *\retval E_OK 表示成功。
 */
e_int32 fsocket_request_failed(fsocket_t *fs, e_uint8 *msg, e_uint32 mlen,
		e_uint8 *recv_buf, e_uint32 recv_len, const e_uint8 *failed_reply,
		const e_uint32 rlen, e_uint32 timeout_usec)
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
		(STDOUT, "[%s_%u]FACK SOCKET send error:msg is too long...\r\n", fs->name, (unsigned int) fs->id));
		return E_ERROR;
	}

	DMSG(
	(STDOUT, "[%s_%u]FACK SOCKET send a msg ...\r\n", fs->name, (unsigned int) fs->id));

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();
	sprintf((char*) buf, "#%02X%02X%s@", (unsigned int) fs->id,
			(unsigned int) ++fs->rq_id, msg);

	DMSG(
	(STDOUT, "++++++++++++++++[%s_%u_%u]FACK SOCKET send a msg[%s] ...\r\n", fs->name, (unsigned int) fs->id, (unsigned int) fs->rq_id, buf));

	//请求发送锁
	//DMSG((STDOUT, "[%s_%u]FACK SOCKET acquaire send sem...\r\n", fs->name, (unsigned int)fs->id));
	ret = semaphore_timeoutwait(&fs->send_sem, timeout_usec);
	e_assert(ret, E_ERROR_INVALID_CALL);

	//DMSG((STDOUT, "[%s_%u]FACK SOCKET request try send ...\r\n", fs->name, (unsigned int)fs->id));
	while (fs->state)
	{
		elapsed_time = GetTickCount() - beg_time;
		if (elapsed_time >= timeout_usec)
				{
			DMSG(
			(STDOUT, "[%s_%u]FACK SOCKET send timeout...\r\n", fs->name, (unsigned int) fs->id));
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
		(STDOUT, "[%s_%u]FACK SOCKET send_one_msg done.\r\n", fs->name, (unsigned int) fs->id));
		return E_OK;
	}

	return E_ERROR_INVALID_STATUS;
}

static e_uint32 wait_for_reply(fsocket_t *fs, e_uint32 timeout_usec)
{
	e_int32 ret;

	DMSG((STDOUT, "[%s_%u]FACK SOCKET wait for reply ...\r\n", fs->name, (unsigned int) fs->id));
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
	(STDOUT, "[%s_%u]FACK SOCKET wait for reply ...\r\n", fs->name, (unsigned int) fs->id));
	//等待消息信号
	ret = semaphore_wait(&fs->recv_sem);
	e_assert(ret, E_ERROR);
	//上层在得到应答后，要马上将缓存的消息拷贝出去，否则会被后来的信息覆盖
	return E_OK;
}

