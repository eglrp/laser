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


/*结构体定义*/
typedef struct scan_job_t* scan_job;

/*接口定义*/
#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/* 与扫描仪建立连接，并初始化扫描参数信息                                                                    */
/************************************************************************/
e_int32 DEV_EXPORT sj_create(scan_job* sj);

/************************************************************************/
/* 与扫描仪断开连接                                                               */
/************************************************************************/
e_int32 DEV_EXPORT sj_destroy(scan_job sj);

/************************************************************************
 * 开始扫描,此时开启读写两个线程
 * 读线程负责读取扫描数据，并写入共享队列
 * 写线程负责读取共享队列数据，并写入到文件
 ************************************************************************/
e_int32 DEV_EXPORT sj_start(scan_job sj);

/************************************************************************
 * 停止扫描
 ************************************************************************/
e_int32 DEV_EXPORT sj_stop(scan_job sj);

e_int32 DEV_EXPORT sj_wait(scan_job sj);

/************************************************************************
 *\brief 设置扫描速度范围参数
 *\param scan_job 定义了扫描任务。
 *\param speed_h 定义了水平速度。
 *\param start_angle_h 定义了水平开始角度。
 *\param end_angle_h 定义了水平结束角度。
 *\param speed_v 定义了垂直速度。
 *\param resolution_v 定义了垂直分辨率。
 *\param active_sector_start_angles 定义了扫描区域开始角度。
 *\param active_sector_stop_angles 定义了扫描区域结束角度。
 *\retval E_OK 表示成功。
 ************************************************************************/
e_int32 DEV_EXPORT sj_config(scan_job sj, e_uint32 speed_h_delay,
		const e_float64 start_angle_h, const e_float64 end_angle_h,
		e_uint32 speed_v_hz, e_float64 resolution_v, const e_float64 start_angle_v,
		const e_float64 end_angle_v);

/************************************************************************
 * 设置点云数据存储目录,灰度图存储目录
 ************************************************************************/
e_int32 DEV_EXPORT sj_set_data_dir(scan_job sj, char* ptDir,
		e_uint8 *grayDir);

#ifdef __cplusplus
}
#endif

#endif /*HD_LASER_SCAN_H*/
