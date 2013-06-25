/*!
 * \file test_laser_scan.c
 * \brief laser scan test
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#if TEST_SCAN
#include <signal.h>
#include <ls300/hd_laser_scan.h>

scan_job job;

void sig_handler(int sig)
{
	if (sig == SIGINT)
	{
		sj_stop(job);
		sj_destroy(job);
		exit(0);
	}
}

int main()
{
	signal(SIGINT, sig_handler);
	int ret;
	DMSG((STDOUT,"LASER scan TEST start.\r\n"));
	ret = sj_create(&job);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER scan TEST config.\r\n"));

	//ret = sj_config(job, 50, 180, 360, 5, 0.5, -45, 0);
	//ret = sj_config(job, 50, 0, 360, 5, 0.5, 0, 90);//???频繁出现取数据错误?sick 3030
	//ret = sj_config(job, 50, 0, 360, 5, 0.5, 0, 85);
	ret = sj_config(job, 50, 0, 90, 5, 0.5, -45, -20);
	//ret = sj_config(job, 50, 160, 200, 5, 0.5, -45, 90);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER scan TEST start.\r\n"));
	ret = sj_start(job);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER scan TEST start.\r\n"));
	ret = sj_wait(job);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER scan TEST PASSED.\r\n"));
	sj_destroy(job);
}
#endif
