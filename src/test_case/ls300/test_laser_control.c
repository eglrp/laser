/*!
 * \file test_laser_control.c
 * \brief laser control test
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
#if TEST_CONTROL
#include <ls300/hd_laser_control.h>

int main()
{
	int ret;
	laser_control_t control;
	DMSG((STDOUT,"LASER CONTROL TEST start.\r\n"));
#if 0
	ret = hl_open_socket(&control,"192.168.1.10",49152);
#else
	ret = hl_open(&control, "/dev/ttyUSB0", 38400);
#endif
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER CONTROL TEST turn.\r\n"));
//	ret = hl_SearchZero(&control);
//	e_assert(ret>0, ret);
//	getchar();
	ret = hl_turntable_turn(&control, 100);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"LASER CONTROL TEST PASSED.\r\n"));
	hl_close(&control);

	Delay(100);

}
#endif
