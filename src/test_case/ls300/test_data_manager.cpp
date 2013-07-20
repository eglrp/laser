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

#include <ls300/hd_data_manager.h>

#if TEST_DATA_MANAGER

#include <stdio.h>
#include <math.h>
#include <comm/hd_point_type.h>

#define SIZE 180
point_t *point_array;

point_xyz_t *points;
point_gray_t *points_gray;

int main()
{
	int ret;

	point_array = malloc_points(PNT_TYPE_XYZ,SIZE);
	points = (point_xyz_t*)point_array->mem;

	data_manager_t *dm;
	char *file_name[] = { "test.pcd", "test.jpg", "test.gif" ,"/tmp/test.sprite"};

	dm = dm_alloc(file_name,4,SIZE,SIZE,E_DWRITE);

	for (int i = 0; i < SIZE; i++) {
		points[i].x = SIZE * sin(i * M_PI / SIZE);
		points[i].y = SIZE * cos(i * M_PI / SIZE);
		points[i].z = SIZE * sin(M_PI - i * M_PI / SIZE);
	}

	for (int i = 0; i < SIZE; i++)
		dm_append_points(dm, point_array, SIZE, 0);

	for (int i = 0; i < SIZE; i++) {
		points[i].x += 100;
		points[i].y += 100;
		points[i].z += 100;
	}

	for (int i = 0; i < SIZE; i++)
		dm_append_points(dm, point_array, SIZE, 1);

	free_points(point_array);


//test2
	point_array = malloc_points(PNT_TYPE_GRAY,SIZE);
	points_gray = (point_gray_t*)point_array->mem;

	for (int i = 0; i < SIZE; i++)
		dm_append_points(dm, point_array, SIZE, 0);

	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = (128 + 255 / SIZE * i) % 255;

	for (int i = 0; i < SIZE; i++)
		dm_append_points(dm, point_array, SIZE, 1);

////test3
//	for (int i = 0; i < SIZE; i++)
//		points_gray[i].gray = 255 * i / SIZE;
//
//	for (int i = 0; i < SIZE; i++)
//		dm_append_row(dm, point_array, 0);
//
//	for (int i = 0; i < SIZE; i++)
//		points_gray[i].gray = (128 + 255 / SIZE * i) % 255;
//
//	for (int i = 0; i < SIZE; i++)
//		dm_write_row(dm, SIZE - i - 1, point_array, 1);

////test4
//	for (int i = 0; i < SIZE; i++)
//		points_gray[i].gray = 'A';
//
//	for (int i = 0; i < SIZE; i++)
//		dm_write_column(dm, i, point_array, 0);
//
//	for (int i = 0; i < SIZE; i++)
//		points_gray[i].gray = 'B';
//
//	for (int i = 0; i < SIZE; i++)
//		dm_write_column(dm, SIZE - i -1, point_array, 1);


	free_points(point_array);

	dm_free(dm);
	printf("ALL TEST PASSED!\n");
	return 0;
}

#endif
