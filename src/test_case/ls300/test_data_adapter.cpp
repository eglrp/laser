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

#include "../hd_test_config.h"
#if TEST_ADAPTER
#include <ls300/hd_data_adapter.h>
#include <stdio.h>
#include <math.h>
#include <comm/hd_point_type.h>

#define W 	721
#define H	541
point_t *point_array;
point_gray_t *points_gray;
point_xyz_t *points;

int main()
{
	data_adapter_t da;
	unsigned char file_name[][64] = { "test.pcd1", "test.jpg", "test.gif",
			"/data/local/tmp/test.sprite" };

	da_open(&da, file_name[3], W, H, E_DWRITE); //data_adapter_t *da, e_uint8 *name, int mode

	point_array = malloc_points(PNT_TYPE_GRAY,H);
	points_gray = (point_gray_t*)point_array->mem;

	for (int i = 0; i < H; i++)
		points_gray[i].gray = 80;

	for (int i = 0; i < W; i++)
		da_write_column(&da,i, point_array, 0);

	for (int i = 0; i < H; i++)
		points_gray[i].gray = 160;

	for (int i = 0; i < W; i++)
		da_write_column(&da, W - i - 1, point_array, 1);

//	point_array = malloc_points(PNT_TYPE_XYZ, H);
//	points = (point_xyz_t*) point_array->mem;
//
//	for (int i = 0; i < H; i++) {
//		points[i].x = H * sin(i * M_PI / H);
//		points[i].y = H * cos(i * M_PI / H);
//		points[i].z = H * sin(M_PI - i * M_PI / H);
//	}
//
//	for (int i = 0; i < W; i++)
//		da_append_points(&da, point_array, H, 0);
//
//	for (int i = 0; i < H; i++) {
//		points[i].x += 100;
//		points[i].y += 100;
//		points[i].z += 100;
//	}
//
//	for (int i = 0; i < W; i++)
//		da_append_points(&da, point_array, H, 1);
//
	da_close(&da);
	free_points(point_array);
	printf("ALL TEST PASSED!\n");
	return 0;
}

#endif
