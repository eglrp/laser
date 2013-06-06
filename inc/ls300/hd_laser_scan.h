/*!
 * \file hd_laser_scan.h
 * \brief	扫描总控
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd laser base
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_LASER_SCAN_H
#define HD_LASER_SCAN_H

/* Dependencies */
#include <arch/hd_plat_base.h>
#include <las/HLSWriter.h>
#include "hd_scan_data_pool.h"
#include "hd_laser_control.h"
#include "sickld/sickld.h"

/*结构体定义*/

typedef struct scan_job_t
{
	sickld_t *sick;
	laser_control_t control;
	scan_pool_t pool;
	ethread_t *thread_read;
	ethread_t *thread_write;

	//配置参数记录
	//垂直方向
	e_uint32 height;
	e_uint32 speed_v; // hz
	e_uint32 resolution_v;
	e_float64 start_angle_v[2]; //垂直起始角度，这里暂时固定分为两个扇区
	e_float64 end_angle_v[2]; //垂直终止角度
	//水平方向
	e_uint32 width;
	e_uint32 speed_h; // s
	e_float64 start_angle_h; //水平起始角度
	e_float64 end_angle_h; //水平终止角度
	struct
	{
		e_uint8 left;
		e_uint8 right;
	} active_sectors;

	//文件读写
	char data_dir[MAX_PATH_LEN];
	hd::HLSWriter hlsWrite;
	char gray_dir[MAX_PATH_LEN];

	//缓冲区
	hd::HLSPointStru *points;
	e_uint8 *gray;

	//临时记录
	e_uint32 slip_idx;

	e_int32 state;
} scan_job_t;

/*接口定义*/
#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/* 与扫描仪建立连接，并初始化扫描参数信息                                                                    */
/************************************************************************/
e_int32 DEV_EXPORT sj_init(scan_job_t* sj);

/************************************************************************/
/* 与扫描仪断开连接                                                               */
/************************************************************************/
e_int32 DEV_EXPORT sj_destroy(scan_job_t *sj);

/************************************************************************
 * 开始扫描,此时开启读写两个线程
 * 读线程负责读取扫描数据，并写入共享队列
 * 写线程负责读取共享队列数据，并写入到文件
 ************************************************************************/
e_int32 DEV_EXPORT sj_start(scan_job_t *sj);

/************************************************************************
 * 停止扫描
 ************************************************************************/
e_int32 DEV_EXPORT sj_stop(scan_job_t *sj);

/************************************************************************
 * 设置扫描速度范围参数
 ************************************************************************/
e_int32 DEV_EXPORT sj_config(scan_job_t *sj, e_uint32 speed_h,
		const e_float64 start_angle_h, const e_float64 end_angle_h,
		e_uint32 speed_v, e_float64 resolution_v, const e_float64 start_angle_v,
		const e_float64 end_angle_v);

/************************************************************************
 * 设置点云数据存储目录,灰度图存储目录
 ************************************************************************/
e_int32 DEV_EXPORT sj_set_data_dir(scan_job_t *sj, char* ptDir,
		e_uint8 *grayDir);

/************************************************************************
 *         根据当前水平角度范围和垂直角度范围，得到每一圈实际扫描数据和灰度图
 ************************************************************************/
e_int32 DEV_EXPORT sj_filter_data(scan_job_t *sj, scan_data_t * pdata);

#ifdef __cplusplus
}
#endif

#endif /*HD_LASER_SCAN_H*/
