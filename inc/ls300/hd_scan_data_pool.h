/*!
 * \file hd_scan_data_pool.h
 * \brief data pool
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd data pool
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_SCAN_DATA_POOL_H
#define HD_SCAN_DATA_POOL_H

/* Dependencies */
#include "sickld/sickld_base.h"
#include <comm/hd_list.h>
#include <arch/hd_thread_api.h>

#define DATA_BUFFER_MAX_NUM		10
#define POOL_SLEEP_TIMEOUT 		(1000*1000)
/*结构体定义*/
//返回给上层的数据格式
typedef struct scan_data_t {
	e_float32 h_angle; //水平转台当前角度,不需要双精度
	e_uint32 num_values[SICK_MAX_NUM_SECTORS]; //对应的每个扇区的点个数，这里暂时扇区数固定为2
	e_uint32 sector_ids[SICK_MAX_NUM_SECTORS];
	e_uint32 data_offsets[SICK_MAX_NUM_SECTORS]; //对应的每个扇区的点偏移
	e_uint32 echo_values[SICK_MAX_NUM_MEASUREMENTS]; //扫描仪扫描一圈的回波信息
	e_float64 range_values[SICK_MAX_NUM_MEASUREMENTS]; //扫描仪扫描一圈的距离信息
	e_float64 scan_angles[SICK_MAX_NUM_MEASUREMENTS]; //扫描仪扫描一圈的角度信息
	e_uint32 sector_start_timestamps; //每一圈开始时间
	e_uint32 sector_stop_timestamps; //每一圈结束时间
} scan_data_t;

typedef struct {
	struct list_head list_node;
	scan_data_t data;
} scan_pool_data_t;

typedef struct scan_pool_t {
	struct list_head *read_node, *write_node;
	scan_pool_data_t buffer_mem[DATA_BUFFER_MAX_NUM];
	semaphore_t sem_read;
	semaphore_t sem_write;
	volatile e_int32 state;
} scan_pool_t;

typedef void on_scan_data(scan_data_t* data);

/* 接口定义 */
#ifdef __cplusplus
extern "C" {
#endif

void DEV_EXPORT pool_init(scan_pool_t* pool);
void DEV_EXPORT pool_cancle(scan_pool_t* pool);
//void DEV_EXPORT pool_leave(scan_pool_t* pool);
void DEV_EXPORT pool_destroy(scan_pool_t* pool);
e_int32 DEV_EXPORT pool_read(scan_pool_t* pool, scan_data_t *data);
e_int32 DEV_EXPORT pool_write(scan_pool_t* pool, scan_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /*HD_SCAN_DATA_POOL_H*/
