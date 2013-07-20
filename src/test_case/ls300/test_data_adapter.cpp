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

#if TEST_ADAPTER

#include <stdio.h>
#include <math.h>
#include <ls300/hd_data_adapter.h>
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

	data_adapter_t *da;
	unsigned char file_name[][20] = { "test.pcd", "test.jpg", "test.gif" ,"/tmp/test.sprite"};

	da = (data_adapter_t*) malloc(sizeof(data_adapter_t));


	memset(da, 0, sizeof(data_adapter_t));

	ret = da_open(da, file_name[0], SIZE, SIZE, E_DWRITE); //data_adapter_t *da, e_uint8 *name, int mode

	for (int i = 0; i < SIZE; i++) {
		points[i].x = SIZE * sin(i * M_PI / SIZE);
		points[i].y = SIZE * cos(i * M_PI / SIZE);
		points[i].z = SIZE * sin(M_PI - i * M_PI / SIZE);
	}

	for (int i = 0; i < SIZE; i++)
		da_append_points(da, point_array, SIZE, 0);

	for (int i = 0; i < SIZE; i++) {
		points[i].x += 100;
		points[i].y += 100;
		points[i].z += 100;
	}

	for (int i = 0; i < SIZE; i++)
		da_append_points(da, point_array, SIZE, 1);

	da_close(da);
	free_points(point_array);


//test2
	point_array = malloc_points(PNT_TYPE_GRAY,SIZE);
	points_gray = (point_gray_t*)point_array->mem;
	memset(da, 0, sizeof(data_adapter_t));
	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = 255 * i / SIZE;

	ret = da_open(da, file_name[1], SIZE, SIZE, E_DWRITE); //data_adapter_t *da, e_uint8 *name, int mode

	for (int i = 0; i < SIZE; i++)
		da_append_points(da, point_array, SIZE, 0);

	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = (128 + 255 / SIZE * i) % 255;

	for (int i = 0; i < SIZE; i++)
		da_append_points(da, point_array, SIZE, 1);

	da_close(da);

//test3
	memset(da, 0, sizeof(data_adapter_t));
	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = 255 * i / SIZE;

	ret = da_open(da, file_name[2], SIZE, SIZE, E_DWRITE); //data_adapter_t *da, e_uint8 *name, int mode

	for (int i = 0; i < SIZE; i++)
		da_append_row(da, point_array, 0);

	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = (128 + 255 / SIZE * i) % 255;

	for (int i = 0; i < SIZE; i++)
		da_write_row(da, SIZE - i - 1, point_array, 1);

	da_close(da);

//test4
	memset(da, 0, sizeof(data_adapter_t));
	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = 'A';

	ret = da_open(da, file_name[3], SIZE, SIZE, E_DWRITE);

	for (int i = 0; i < SIZE; i++)
		da_write_column(da, i, point_array, 0);

	for (int i = 0; i < SIZE; i++)
		points_gray[i].gray = 'B';

	for (int i = 0; i < SIZE; i++)
		da_write_column(da, SIZE - i -1, point_array, 1);

	da_close(da);
	free_points(point_array);

	free(da);
	printf("ALL TEST PASSED!\n");
	return 0;
}

#endif
