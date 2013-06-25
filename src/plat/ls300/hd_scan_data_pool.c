/*!
 * \file hd_scan_data_pool.c
 * \brief data pool
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd data pool
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#include <ls300/hd_scan_data_pool.h>
#include <arch/hd_timer_api.h>

#ifdef DMSG
#undef DMSG
#define DMSG
#endif

//----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/**
 *\brief 初始化共享队列数据。
 *\param pool 定义了共享队列对象。
 *\retval E_OK 表示成功。
 */
void pool_init(scan_pool_t* pool)
{
	e_uint32 i;
	memset(pool, 0, sizeof(scan_pool_t));
	INIT_LIST_HEAD(&pool->buffer_mem[0].list_node);
	// dispose
	for (i = 1; i < DATA_BUFFER_MAX_NUM; i++)
			{
		// add to list
		list_add_after(&pool->buffer_mem[i].list_node,
						&pool->buffer_mem[0].list_node);
	}
	pool->read_node = pool->write_node = &pool->buffer_mem[0].list_node;
	semaphore_init(&pool->sem_read, 0);
	semaphore_init(&pool->sem_write, DATA_BUFFER_MAX_NUM);
	pool->state = 1;
}

/**
 *\brief 设置共享队列为无效。
 *\param pool 定义了共享队列对象。
 *\retval E_OK 表示成功。
 */
void pool_cancle(scan_pool_t* pool)
{
	e_assert(pool&&pool->state);
	pool->state = 2;
}

void pool_leave(scan_pool_t* pool)
{
	e_assert(pool&&pool->state);
	while (pool->read_node != pool->write_node && pool->state == 1)
		Delay(100);
	pool->state = 2;
}

/**
 *\brief 销毁共享队列。
 *\param pool 定义了共享队列对象。
 *\retval E_OK 表示成功。
 */
void pool_destroy(scan_pool_t* pool)
{
	if (pool->state)
	{
		pool->state = 0;
		semaphore_destroy(&pool->sem_read);
		semaphore_destroy(&pool->sem_write);
		memset(pool, 0, sizeof(scan_pool_t));
	}
}

static void pool_push(scan_pool_t* pool, scan_data_t *data)
{
	scan_pool_data_t *pdata;
	pdata = list_entry(pool->write_node,scan_pool_data_t,list_node);
	memcpy(&pdata->data, data, sizeof(scan_data_t));
	pool->write_node = pool->write_node->next;
	DMSG((STDOUT, "pool cache push a data.\r\n"));
}

static void pool_pop(scan_pool_t* pool, scan_data_t *data)
{
	scan_pool_data_t * pdata;
	pdata = list_entry(pool->read_node,scan_pool_data_t,list_node);
	memcpy(data, &pdata->data, sizeof(scan_data_t));
	pool->read_node = pool->read_node->next;
	DMSG((STDOUT, "pool cache pop a data\r\n"));
}

/**
 *\brief 从共享队列读数据。
 *\param pool 定义了共享队列对象。 
 *\param data 定义了队列的数据元素。
 *\retval E_OK 表示成功。
 */
e_int32 pool_read(scan_pool_t* pool, scan_data_t *data)
{
	e_int32 ret;
	e_assert(pool&&pool->state, E_ERROR_INVALID_HANDLER);
	while (pool->state == 1)
	{
		ret = semaphore_timeoutwait(&pool->sem_read, POOL_SLEEP_TIMEOUT);
		if (ret > 0)
			break;
		if (ret == E_ERROR_INVALID_HANDLER)
			return ret;
	}

	if (pool->state != 1)
		return E_ERROR;
	pool_pop(pool, data);
	ret = semaphore_post(&pool->sem_write);
	e_assert(ret, E_ERROR);
	if (pool->state != 1)
		return E_ERROR;
	return E_OK;
}

/**
 *\brief 把数据写入共享队列。
 *\param pool 定义了共享队列对象。 
 *\param data 定义了队列的数据元素。
 *\retval E_OK 表示成功。
 */
e_int32 pool_write(scan_pool_t* pool, scan_data_t *data)
{
	e_int32 ret;
	e_assert(pool&&pool->state, E_ERROR_INVALID_HANDLER);
	while (pool->state == 1)
	{
		ret = semaphore_timeoutwait(&pool->sem_write, POOL_SLEEP_TIMEOUT);
		if (ret > 0)
			break;
		if (ret == E_ERROR_INVALID_HANDLER)
			return ret;
	}
	if (pool->state != 1)
		return E_ERROR;
	pool_push(pool, data);
	ret = semaphore_post(&pool->sem_read);
	e_assert(ret, E_ERROR);
	if (pool->state != 1)
		return E_ERROR;
	return E_OK;
}
