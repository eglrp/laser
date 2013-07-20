/*!
 * \file hd_data_adapter.h
 * \brief 数据存取适配器
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_DATA_ADAPTER_H
#define HD_DATA_ADAPTER_H

#include <arch/hd_plat_base.h>
#include <comm/hd_point_type.h>

#define POINT_NUM_BUF        (10*1024)
#define FILE_SUFFIX_LENGTH   (16)


typedef struct file_info_t
{
	char file_name[MAX_PATH_LEN];
	e_uint32 width;
	e_uint32 height;
	e_int32 pnt_type;
	int mode;
} file_info_t;

typedef struct file_ptr_t {
	size_t handle;
	e_uint8 modify;
	e_uint8 is_main;
	e_uint32 cousor;

	file_info_t info;
} file_ptr_t;

typedef struct
{
	e_int32 (*open)(file_ptr_t *file);
	e_int32 (*combine)(file_ptr_t *file1, file_ptr_t *file2);
	e_int32 (*on_data_change)(file_ptr_t *file1, file_ptr_t *file2);
	e_int32 (*close)(file_ptr_t *file);
	e_int32 (*write_point)(file_ptr_t *file, int x, int y, point_t* point);
	e_int32 (*write_row)(file_ptr_t *file, e_uint32 row_idx, point_t* point);
	e_int32 (*write_column)(file_ptr_t *file, e_uint32 column_idx, point_t* point);
	e_int32 (*append_points)(file_ptr_t *file, point_t* point, int pt_num);
	e_int32 (*append_row)(file_ptr_t *file, point_t* point);
	e_int32 (*append_column)(file_ptr_t *file, point_t* point);
	e_int32 (*read_points)(file_ptr_t *file, point_t* point, int buf_len);
	e_int32 (*read_row)(file_ptr_t *file, e_uint32 row_idx, point_t* buf);
	e_int32 (*read_column)(file_ptr_t *file, e_uint32 column_idx, point_t* buf);
} file_op_t;

typedef struct data_adapter_t
{
	file_info_t file_info;
	struct {
		union {
			struct {
				file_ptr_t left;
				file_ptr_t right;
			} write;
			file_ptr_t read;
		};
	} file;

	e_int32 pnt_type;
	file_op_t op;
	e_int32 state;
} data_adapter_t;

typedef struct {
	char file_suffix[FILE_SUFFIX_LENGTH];
	e_int32 pnt_type;
	file_op_t op;
} file_adapter_t;

/* 接口定义 */
#ifdef __cplusplus
extern "C"
{
#endif
////////////////////////////////////////////////////////////////////////
//用户用
e_int32 DEV_EXPORT da_open(data_adapter_t *da, e_uint8 *name, int width, int height,
		int mode);/*E_READ,E_WRITE*/
e_int32 DEV_EXPORT da_close(data_adapter_t *da);

e_int32 DEV_EXPORT da_write_point(data_adapter_t *da, int x, int y, point_t* point,
		int file_right);
e_int32 DEV_EXPORT da_write_row(data_adapter_t *da, e_uint32 row_idx, point_t* point,
		int file_right);
e_int32 DEV_EXPORT da_write_column(data_adapter_t *da, e_uint32 column_idx,
		point_t* point,
		int file_right);
e_int32 DEV_EXPORT da_append_points(data_adapter_t *da, point_t* point, int pt_num,
		int file_right);
e_int32 DEV_EXPORT da_append_row(data_adapter_t *da, point_t* point,
		int file_right);
e_int32 DEV_EXPORT da_append_column(data_adapter_t *da, point_t* point,
		int file_right);
e_int32 DEV_EXPORT da_read_header(data_adapter_t *da, file_info_t *header);
e_int32 DEV_EXPORT da_read_points(data_adapter_t *da, point_t* point, int buf_len);
e_int32 DEV_EXPORT da_read_row(data_adapter_t *da, e_uint32 row_idx, point_t* buf);
e_int32 DEV_EXPORT da_read_column(data_adapter_t *da, e_uint32 column_idx, point_t* buf);

#ifdef __cplusplus
}
#endif

#endif /*HD_DATA_ADAPTER_H*/
