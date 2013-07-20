/*!
 * \file hd_data_manager.h
 * \brief 数据存取
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_DATA_MANAGER_H
#define HD_DATA_MANAGER_H

#include <ls300/hd_data_adapter.h>


typedef struct data_manager_t
{
	e_int32 state;
	e_uint32 num;
	data_adapter_t adapters[];
} data_manager_t;


/* 接口定义 */
#ifdef __cplusplus
extern "C"
{
#endif
////////////////////////////////////////////////////////////////////////
//用户用
data_manager_t* DEV_EXPORT dm_alloc(char **files,int num,int width,int height,int mode);/*E_READ,E_WRITE*/
e_int32 DEV_EXPORT dm_free(data_manager_t *da);

e_int32 DEV_EXPORT dm_write_point(data_manager_t *da, int x, int y, point_t* point,
		int file_right);
e_int32 DEV_EXPORT dm_write_row(data_manager_t *da, e_uint32 row_idx, point_t* point,
		int file_right);
e_int32 DEV_EXPORT dm_write_column(data_manager_t *da, e_uint32 column_idx,
		point_t* point,
		int file_right);
e_int32 DEV_EXPORT dm_append_points(data_manager_t *da, point_t* point, int pt_num,
		int file_right);
e_int32 DEV_EXPORT dm_append_row(data_manager_t *da, point_t* point,
		int file_right);
e_int32 DEV_EXPORT dm_append_column(data_manager_t *da, point_t* point,
		int file_right);

#ifdef __cplusplus
}
#endif

#endif /*HD_DATA_MANAGER_H*/
