/*!
 * \file test_data_pool.c
 * \brief data pool test
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#include "../hd_test_config.h"
#if TEST_POOL
#include "hd_scan_data_pool.h"
#include <hd_thread_api.h>
#include <hd_timer_api.h>

#define DATA_NUMBER 1024
volatile count=0;

static void write_pool_data_routine(void* data)
{
	e_int32 ret,i;
	scan_data_t sdata;
	scan_pool_t *pool = (scan_pool_t *) data;
	DMSG((STDOUT,"scan job:write_data_routine start.\r\n"));
	//TODO:更详细的异常处理
	for(i=0;i<DATA_NUMBER;i++)
	{
		sdata.h_angle = (e_float64)i;
		ret = pool_write(pool, &sdata);
		if (ret <= 0)
		{
			break;
		}
	}
	DMSG((STDOUT,"scan job:write_data_routine done."));
	count++;
	Delay(100);
	pool_cancle(pool);//终止队列
}

static void read_pool_data_routine(void* data)
{
	e_int32 ret,i=0;
	scan_data_t sdata;
	scan_pool_t *pool = (scan_pool_t *) data;
	DMSG((STDOUT,"scan job:read_data_routine start.\r\n"));
	//TODO:更详细的异常处理
	while (1)
	{
		ret = pool_read(pool, &sdata);
		if (ret <= 0)
		{
			break;
		}
		DMSG((STDOUT,"read_data_routine write data at angle: %f.\r\n",(float)sdata.h_angle));
	}
	DMSG((STDOUT,"scan job:write_data_routine done.\r\n"));
	count++;
}

int main(int argc, char *argv[])
{
	e_int32 ret,i;
	scan_pool_t pool;

	pool_init(&pool);

	ethread_t* threads[2];
	DMSG((STDOUT, "scan job routine starting write routine...\r\n"));
	ret = createthread("hd_scan_job_write", (thread_func) &write_pool_data_routine,
			&pool, NULL, &threads[0]);
	if (ret <= 0)
	{
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(threads[0]);
	if (ret <= 0)
	{
		DMSG((STDOUT, "write resumethread failed!\r\n"));
		return E_ERROR;
	}
	DMSG((STDOUT, "scan job routine starting read routine...\r\n"));
	ret = createthread("hd_scan_job_read", (thread_func) &read_pool_data_routine,
			&pool,NULL,  &threads[1]);
	if (ret <= 0)
	{
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(threads[1]);
	if (ret <= 0)
	{
		DMSG((STDOUT, "read resumethread failed!\r\n"));
		return E_ERROR;
	}

	while(count<2)
	{
		//DMSG((STDOUT,"COUNT = %d",count));
		sleep(1);
	}

	for (i=0;i<2;i++)
	{
		killthread( threads[i]);
	}
	pool_destroy(&pool);
	return 0;
}

#endif
