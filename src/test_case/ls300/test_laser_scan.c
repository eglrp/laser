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
#include "hd_laser_control.h"

int main(){
	int ret;
	laser_control_t control;
	DMSG((STDOUT,"LASER CONTROL TEST start.\r\n"));
	ret = hl_open_socket(&control,"127.0.0.1",6666);
	e_assert(ret>0,ret);

	DMSG((STDOUT,"LASER CONTROL TEST turn.\r\n"));
	ret = hl_turntable_turn(&control,60);
	e_assert(ret>0,ret);

	DMSG((STDOUT,"LASER CONTROL TEST PASSED.\r\n"));

}
#endif
