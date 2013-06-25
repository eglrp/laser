/*!
 * \file hd_message_monitor.c
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

#include <stdio.h>
#include <comm/hd_list.h>
#include <arch/hd_timer_api.h>
#include <arch/hd_thread_api.h>
#include <ls300/hd_message_monitor.h>

#ifdef DMSG
#undef DMSG
#define DMSG
#endif

enum
{
	STATE_NONE = 0, STATE_INIT = 1, STATE_START = 2, STATE_STOP = 3,
};

#define  	MSG_END 	'@'
#define 	MSG_START	'#'
#define 	SESSION_ID_INVALID -1
#define 	MAX_TRY_COUNT ((e_uint32)50)
//----------------------------------------------------------------------------

e_int32 mm_loop(void *data);

/**
 *\brief 监控线程创建网络连接。
 *\param msg_monitor_t 定义了监控线程。
 *\param name 定义了监控线程的标识符。
 *\retval E_OK 表示成功。
 */
fsocket_t*
mm_create_socket(msg_monitor_t *mm, e_uint8* name)
{
	e_int32 ret;
	e_assert(mm&&mm->state, E_ERROR);
	e_assert(mm->sockets_num<MAX_CLIENT_SIZE, E_ERROR);
	ret = fsocket_open(&mm->sockets[mm->sockets_num], name, mm->sockets_num,
						mm->connect);
	e_assert(ret>0, E_ERROR);
	mm->sockets_num++;
	DMSG((STDOUT, "MSG_MONITOR add socket:%s, sockets_num=%u\r\n",
			name, (unsigned int) mm->sockets_num));
	return &mm->sockets[mm->sockets_num - 1];
}

e_int32 mm_destroy_socket(msg_monitor_t *mm, fsocket_t* fs)
{
	e_assert(mm&&mm->state, E_ERROR_INVALID_HANDLER);
	e_assert(fs->id>=0 && fs->id<MAX_CLIENT_SIZE, E_ERROR);
	fsocket_close(fs);
	mm->sockets_num--;
	DMSG((STDOUT, "MSG_MONITOR remove socket:%s, sockets_num=%u\r\n",
			fs->name, fs->id));
	return E_OK;
}

e_int32 mm_init(msg_monitor_t *mm, hd_connect_t *connect)
{
	DMSG((STDOUT, "SESSION MONITOR INITING...\r\n"));
	memset(mm, 0, sizeof(msg_monitor_t));
	mm->connect = connect;
	mm->state = STATE_INIT;
	DMSG((STDOUT, "SESSION MONITOR INIT done...\r\n"));
	return 1;
}

e_int32 mm_clean(msg_monitor_t *mm)
{
	DMSG((STDOUT, "SESSION MONITOR clean...\r\n"));
	if (mm->state == STATE_STOP || mm->state == STATE_INIT)
			{
		int num = mm->sockets_num;
		while (num--)
		{
			mm_destroy_socket(mm, &mm->sockets[num]);
		}
		killthread(mm->thread_loop);
		mm->state = STATE_NONE;
	}
	DMSG((STDOUT, "SESSION MONITOR STOPT done...\r\n"));
	return 1;
}

static e_int32 get_one_msg(msg_monitor_t *mm, e_uint8* buf, int buf_len,
		e_uint8 *p_msg_id)
{
	e_uint32 read_count = 0;
	e_uint32 count = 0;
	e_int32 ret;
	int iid;
	e_uint8 c;
	DMSG((STDOUT, "SESSION MONITOR get a msg ...\r\n"));
	while (mm->state != STATE_STOP)
	{
		if (read_count >= buf_len)
				{ //避免缓冲区溢出
			DMSG((STDOUT, "get msg failed:buf_len excude.\r\n"));
			return read_count;
		}
		while (mm->state != STATE_STOP) //不能死等
		{
			ret = sc_select(mm->connect, E_READ, MM_SLEEP_TIMEOUT);
			if (ret > 0)
				break;
			if (ret == E_ERROR_INVALID_HANDLER)
				return ret;
			Delay(10);
		}
		if (mm->state == STATE_STOP)
			break;
		ret = sc_recv(mm->connect, &c, 1);
		if (ret <= 0)
				{
			//TODO:更详细的错误处理
			if (count >= MAX_TRY_COUNT)
			{
				DMSG((STDOUT, "get_one_msg RET TRY FAILED! \r\n"
						"END MAIN LOOP!!!\r\n"));
				return E_ERROR_IO;
			}
			DMSG((STDOUT, "get_one_msg failed,retry...[%u]\r\n", (unsigned int) count));
			Delay(10);
			count++;
			continue;
		}
		else
		{
			count = 0;
		}

		if (read_count == 0)
				{ //找到头
			if (c == MSG_START)
				buf[read_count++] = c;
		}
		else
		{
			if (c == MSG_END)
			{ //找到尾
				buf[read_count++] = MSG_END;
				buf[read_count] = 0;
				if (p_msg_id)
				{
					(*p_msg_id) = SESSION_ID_INVALID;
					//导出消息 id
					if (read_count >= 5)
							{
						sscanf(buf, "#%02X", &iid);
						(*p_msg_id) = iid & 0xFF;
					}
					if ((*p_msg_id) == SESSION_ID_INVALID)
						return E_ERROR;
				}
				DMSG((STDOUT, "SESSION MONITOR get a msg done [ %s ]...\r\n",
						buf));
				return read_count; //得到一个消息，返回
			}
			else if (c == MSG_START)
			{ //怎么又是个头？
				read_count = 0;
				buf[read_count++] = c; //从头再来
			}
			else
			{
				buf[read_count++] = c; //消息主体
			}
		}
	}
	return E_ERROR;
}

/**
 *\brief 开启监控线程。
 *\param msg_monitor_t 定义了监控线程。
 *\retval E_OK 表示成功。
 */
e_int32 mm_start(msg_monitor_t *mm)
{
	e_int32 ret;
	DMSG((STDOUT, "SESSION MONITOR STARTING...\r\n"));
	if (mm->state != STATE_INIT)
		return E_OK;
	mm->state = STATE_START;
	ret = createthread("hd_message_monitor", (thread_func) &mm_loop, mm, NULL,
						&mm->thread_loop);
	if (ret <= 0)
			{
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(mm->thread_loop);
	if (ret <= 0)
			{
		DMSG((STDOUT, "resumethread failed!\r\n"));
		return E_ERROR;
	}
	DMSG((STDOUT, "SESSION MONITOR START done.\r\n"));
	return ret;
}

/**
 *\brief 停止监控线程。
 *\param msg_monitor_t 定义了监控线程。
 *\retval E_OK 表示成功。
 */
e_int32 mm_stop(msg_monitor_t *mm)
{
	DMSG((STDOUT, "SESSION MONITOR STOPTING...\r\n"));
	if (mm->state == STATE_START)
			{
		mm->state = STATE_STOP;
	}
//	while (mm->state != STATE_NONE)
//		Delay(100); //等待子线程退出
	mm_clean(mm);
	return E_OK;
}

e_int32 mm_loop(void *data)
{
	e_int32 ret = E_OK, len;
	e_uint8 session_id;
	struct msg_monitor_t *monitor = (struct msg_monitor_t*) data;
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };

	DMSG((STDOUT, "SESSION MONITOR loop start... mm->state=%d\r\n",
			(int) monitor->state));
	while (monitor->state == STATE_START)
	{
		len = get_one_msg(monitor, buf, sizeof(buf), &session_id); //提取一个消息
		if (len <= 0)
			break;
		//发送消息通知
		strncpy(monitor->sockets[session_id].buf, buf, MSG_MAX_LEN);
		ret = semaphore_post(&monitor->sockets[session_id].recv_sem);
		//TODO:详细的失败处理？
		if (!ret) //失败？
		{
			DMSG((STDOUT, "wake up socket[%u] to read:semaphore post failed.\r\n",
					session_id));
		}
		//继续等待消息
	}
	return E_OK;
}

