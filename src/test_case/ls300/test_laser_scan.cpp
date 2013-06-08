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
#include <ls300/hd_laser_scan.h>

int main()
{
	int ret;
	scan_job job;
	DMSG((STDOUT,"LASER scan TEST start.\r\n"));
	ret = sj_create(&job);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER scan TEST config.\r\n"));
	ret = sj_config(job, 20, 30, 60, 20, 0.5, 0, 80);
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
