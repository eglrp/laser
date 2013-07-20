/*!
 * \file hd_laser_scan.c
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

#include <ls300/hd_laser_scan.h>
#include <ls300/hd_scan_data_pool.h>
#include <ls300/hd_laser_control.h>
#include <sickld/sickld.h>
#include <ls300/hd_data_manager.h>

extern "C" {
#include <jpeg/jpeglib.h>
}

#include <arch/hd_timer_api.h>
#include <comm/hd_utils.h>

struct scan_job_t {
	sickld_t *sick;
	laser_control_t control;
	scan_pool_t pool;
	ethread_t *thread_read;
	ethread_t *thread_write;

	//配置参数记录
	//垂直方向
	e_uint32 height;
	e_uint32 speed_v; // hz
	e_float64 resolution_v;
	e_float64 start_angle_v[2]; //垂直起始角度，这里暂时固定分为两个扇区
	e_float64 end_angle_v[2]; //垂直终止角度
	//水平方向
	e_uint32 width;
	e_uint32 speed_h; // s
	e_float64 start_angle_h; //水平起始角度
	e_float64 end_angle_h; //水平终止角度
	struct {
		e_uint8 left;
		e_uint8 right;
	} active_sectors;

	//文件读写
	char data_file[MAX_PATH_LEN];
	data_manager_t* writer;
	char gray_dir[MAX_PATH_LEN];
	point_t *points_xyz;
	point_t *points_gray;

	//临时记录
	e_uint32 slip_idx;
	e_float32 pre_scan_angle;

	volatile e_int32 state;
};

#define PRE_SCAN_WAIT_TIME  0 //us
#define JPEG_QUALITY 100     //它的大小决定jpg的质量好坏
enum {
	STATE_NONE = 0,
	STATE_INIT = 1,
	STATE_START = 2,
	STATE_STOP = 3,
	STATE_DONE = 4
};
static e_int32 sj_clean(scan_job sj);
static e_int32 sj_create_writer(scan_job sj);
static void scan_done(scan_job sj);
int sj_save2image(char *filename, unsigned char *bits, int width, int height,
		int depth);
static int sj_save2image_rotation(char *filename, unsigned char *bits, int width,
		int height);
/************************************************************************
 *         根据当前水平角度范围和垂直角度范围，得到每一圈实际扫描数据和灰度图
 ************************************************************************/
static e_int32 sj_filter_data(scan_job sj, scan_data_t * pdata);

static void write_pool_data_routine(void* data) {
	e_int32 ret;
	scan_job sj = (scan_job) data;
	scan_data_t sdata = { 0 };
	DMSG((STDOUT,"scan job:write_data_routine start.\r\n"));

	//等待扫描仪就位,会有准备工作开始
	do {
		DMSG((STDOUT,"sld_get_measurements sick is not in target position: delay and retry.\r\n"));
		Delay(1000 / sj->speed_v); //等垂直一圈的时间
		sdata.h_angle = hl_turntable_get_angle(&sj->control) - sj->pre_scan_angle;
		DMSG((STDOUT,"control trun to start angle left  %f",sdata.h_angle));
	} while (sj->state == STATE_START
			&& (sdata.h_angle < -0.5));

	DMSG((STDOUT,"sld_get_measurements sick is in target position!\r\n start get data.\r\n"));

	while (sj->state == STATE_START) {
		//1先读取水平方向角度
		sdata.h_angle = hl_turntable_get_angle(&sj->control) - sj->pre_scan_angle
				+ sj->start_angle_h;

		if (sdata.h_angle > sj->end_angle_h + 0.5) {
			DMSG((STDOUT,"sld_get_measurements sick is out of target position again!\r\n stop get data.\r\n"));
			sj->state = STATE_STOP;
			break;
		}

		ret =
				sld_get_measurements(sj->sick, sdata.range_values,
										sdata.echo_values, sdata.scan_angles, sdata.num_values,
										sdata.sector_ids, sdata.data_offsets, NULL, NULL, NULL, NULL,
										NULL);
		if (ret <= 0) {
			DMSG((STDOUT,"sld_get_measurements ERROR: RETRY.\r\n"));
			Delay(10);
			continue;
		}

		DMSG((STDOUT,"\rWRITE TO FILE: %f end: %f\n",sdata.h_angle,sj->end_angle_h));

		//等待缓冲区空位
		while (pool_write(&sj->pool, &sdata) <= 0) {
			DMSG((STDOUT,"pool_write ERROR: RETRY.\r\n"));
			Delay(10);
		}

	}

	pool_leave(&sj->pool);
	DMSG((STDOUT,"scan job:write_data_routine done.\n"));
}

static void read_pool_data_routine(void* data) {
	e_int32 ret;
	scan_job sj = (scan_job) data;
	scan_data_t sdata;
	DMSG((STDOUT,"scan job:read_data_routine start.\r\n"));
	//TODO:更详细的异常处理
	while (sj->state == STATE_START) {
		ret = pool_read(&sj->pool, &sdata);
		if (ret <= 0) {
			DMSG((STDOUT,"pool_read ERROR: RETRY.\r\n"));
			Delay(10);
			continue;
		}
//		DMSG((STDOUT,"read_data_routine read data at angle: %f.\r\n", sdata.h_angle));
		sj_filter_data(sj, &sdata);
	}
	for (;;) {
		ret = pool_read(&sj->pool, &sdata);
		if (ret <= 0) {
			DMSG((STDOUT,"pool has been cleared, leave!\r\n"));
			break;
		}
//		DMSG((STDOUT,"read_data_routine read data at angle: %f.\r\n", sdata.h_angle));
		sj_filter_data(sj, &sdata);
	}

	if (e_check(sj->slip_idx*(sj->active_sectors.left+sj->active_sectors.right) != sj->width, "#ERROR# 列数与要求的不一致!\n")) {
		DMSG((STDOUT, "sj->slip_idx*sector_num = %u, sj->width = %u \n",
				(unsigned int)sj->slip_idx*(sj->active_sectors.left+sj->active_sectors.right),(unsigned int)sj->width));
		sj->width = sj->slip_idx * (sj->active_sectors.left + sj->active_sectors.right); //TODO:修复这里,不允许简单丢掉！!
	}

	scan_done(sj);
	DMSG((STDOUT,"scan job:write_data_routine done.\r\n"));
}

/**
 *\brief 与扫描仪建立连接，并初始化扫描参数信息。
 *\param scan_job 定义了扫描任务。 
 *\retval E_OK 表示成功。
 */
e_int32 sj_create(scan_job* sj_ret) {
	e_int32 ret;
	scan_job sj;
	sj = (scan_job) malloc(sizeof(struct scan_job_t));
	e_assert(sj, E_ERROR_BAD_ALLOCATE);
	memset(sj, 0, sizeof(scan_job_t));
	ret = hl_open(&sj->control, (char*) "/dev/ttyUSB0", 38400);
	if (e_failed(ret))
		goto FAILED;
	ret = sld_create(&sj->sick, NULL, NULL);
	if (e_failed(ret)) {
		DMSG(
				(STDOUT,"connect to sick scanner failed!\nplease check that you'v set the correct IP address.\n"));
		goto FAILED;
	}
	pool_init(&sj->pool);
	sj->state = 1;
	(*sj_ret) = sj;
	return E_OK;
	FAILED: free(sj);
	return ret;

}

/**
 *\brief 与扫描仪断开连接   
 *\param scan_job 定义了扫描任务。 
 *\retval E_OK 表示成功。
 */
e_int32 sj_destroy(scan_job sj) {
	hl_close(&sj->control);
	pool_destroy(&sj->pool);
	sld_release(&sj->sick);
	sj->state = 0;
	return E_OK;
}

static void pre_start(scan_job sj) {
	e_uint32 gray_size;
//创建数据文件
	sj_create_writer(sj);
//创建缓冲区
	sj->points_xyz = malloc_points(PNT_TYPE_POLAR, sj->height);
//创建色彩缓冲
	gray_size = sj->width * sj->height;
	sj->points_gray =  malloc_points(PNT_TYPE_GRAY, sj->height);

	sj->slip_idx = 0;
}

static void scan_done(scan_job sj) {
	if (sj->state == STATE_STOP) {
		dm_free(sj->writer); //写入文件
		if (sj->points_xyz) {
			free_points(sj->points_xyz);
			sj->points_xyz = NULL;
		}
		if (sj->points_gray) {
			free_points(sj->points_gray);
			sj->points_gray = NULL;
		}

		sj->state = STATE_DONE;
	}
}

static void wait_sick_start(scan_job sj)
		{
	e_int32 ret;
	scan_data_t sdata = { 0 };
	for (;;) {
		ret =
				sld_get_measurements(sj->sick, sdata.range_values,
										sdata.echo_values, sdata.scan_angles, sdata.num_values,
										sdata.sector_ids, sdata.data_offsets, NULL, NULL, NULL, NULL,
										NULL);
		if (ret <= 0) {
			DMSG((STDOUT,"sld_get_measurements wait to start...RETRY.\r\n"));
			Delay(10);
			continue;
		}
		break;
	}
}

/**
 *\brief 开始扫描,此时开启读写两个线程。读线程负责读取扫描数据，并写入共享队列。写线程负责读取共享队列数据，并写入到文件。
 *\param scan_job 定义了扫描任务。 
 *\retval E_OK 表示成功。
 */
e_int32 sj_start(scan_job sj) {
	e_int32 ret;
	DMSG((STDOUT, "scan job routine starting...\r\n"));
	if (sj->state != STATE_INIT)
		return E_OK;
	sj->state = STATE_START;
//等待扫描仪就绪
	ret = sld_initialize(sj->sick);
	if (ret <= 0) {
		DMSG((STDOUT, "hd laser sick start failed!\r\n"));
		return E_ERROR;
	}

	//开始真正数据采集前的准备工作
	ret = hl_turntable_prepare(&sj->control, sj->pre_scan_angle);
	if (ret <= 0) {
		DMSG((STDOUT, "hd laser control prepared failed!\r\n"));
		return E_ERROR;
	}

	pre_start(sj);

	ret =
			sld_set_global_params_and_scan_areas(sj->sick, sj->speed_v,
													sj->resolution_v, sj->start_angle_v, sj->end_angle_v,
													sj->active_sectors.left
															+ sj->active_sectors.right);
	e_assert(ret>0, ret);

	/* Print the sector configuration */
	ret = sld_print_sector_config(sj->sick);
	e_assert(ret>0, ret);

	wait_sick_start(sj);

	ret = hl_turntable_start(&sj->control);
	if (ret <= 0) {
		DMSG((STDOUT, "hd laser control start failed!\r\n"));
		return E_ERROR;
	}
//	Delay(300); //等待控制板开始转动300ms

	DMSG((STDOUT, "scan job routine starting write routine...\r\n"));
	ret = createthread("hd_scan_job_write",
						(thread_func) &write_pool_data_routine, sj, NULL,
						&sj->thread_write);
	if (ret <= 0) {
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(sj->thread_write);
	if (ret <= 0) {
		DMSG((STDOUT, "resumethread write failed!\r\n"));
		return E_ERROR;
	}
	DMSG((STDOUT, "scan job routine starting read routine...\r\n"));
	ret =
			createthread("hd_scan_job_read",
							(thread_func) &read_pool_data_routine, sj, NULL, &sj->thread_read);
	if (ret <= 0) {
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(sj->thread_read);
	if (ret <= 0) {
		DMSG((STDOUT, "resumethread read failed!\r\n"));
		return E_ERROR;
	}
	DMSG((STDOUT, "scan job routine start done.\r\n"));
	return E_OK;
}

/**
 *\brief 停止扫描 
 *\param scan_job 定义了扫描任务。 
 *\retval E_OK 表示成功。
 */
e_int32 sj_stop(scan_job sj) {
	DMSG((STDOUT, "scan job routine stop...\r\n"));
	if (sj->state == STATE_START) {
		sj->state = STATE_STOP;
	}
	sj_clean(sj);
	return E_OK;
}

/**
 *\brief 等待子线程退出。
 *\param scan_job 定义了扫描任务。 
 *\retval E_OK 表示成功。
 */
e_int32 sj_wait(scan_job sj) {
	DMSG((STDOUT, "scan job routine wait job end...\r\n"));
	while (sj->state != STATE_DONE)
		Delay(1000); //等待子线程止
	sj_clean(sj);
	return E_OK;
}

static e_int32 sj_clean(scan_job sj) {
	DMSG((STDOUT, "scan job routine clean...\r\n"));
	if (sj->state == STATE_DONE || sj->state == STATE_STOP) {
		sj->state = STATE_NONE;
		hl_turntable_stop(&sj->control);
		sld_uninitialize(sj->sick);
		killthread(sj->thread_read);
		killthread(sj->thread_write);
	}
	return E_OK;
}

static void print_config(scan_job sj) {
	DMSG((STDOUT,"\t======================================================\n"));
	DMSG((STDOUT,"\tLS300 V2.0 Config\n"));
	DMSG((STDOUT,"\tTurntable:\n"));
	DMSG((STDOUT,"\t\tSpeed:%u ms / step\n",(unsigned int)sj->speed_h));
	DMSG((STDOUT,"\t\tAngle:%f - %f\n",sj->start_angle_h,sj->end_angle_h));
	DMSG((STDOUT,"\tLaser Machine:\n"));
	DMSG((STDOUT,"\t\tSpeed:%u Hz\n",(unsigned int)sj->speed_v));
	DMSG((STDOUT,"\t\tResolution:%f\n",sj->resolution_v));
	DMSG((STDOUT,"\t\tAngle:%f - %f\n",sj->start_angle_v[0],sj->end_angle_v[0]));
	if (sj->active_sectors.left && sj->active_sectors.right)
		DMSG((STDOUT,"\t\tAngle:%f - %f\n",sj->start_angle_v[1],sj->end_angle_v[1]));
	DMSG((STDOUT,"\tOutput:\n"));
	DMSG((STDOUT,"\t\tWidth:%u Height:%u\n",sj->width,sj->height));
	DMSG((STDOUT,"\t======================================================\n"));
}

/**
 *\brief 设置扫描区域        
 *\param scan_job 定义了扫描任务。 
 *\param speed_h 定义了水平速度。 
 *\param start_angle_h 定义了水平开始角度。
 *\param end_angle_h 定义了水平结束角度。
 *\param speed_v 定义了垂直速度。 
 *\param resolution_v 定义了垂直分辨率。
 *\param active_sector_start_angles 定义了扫描区域开始角度。
 *\param active_sector_stop_angles 定义了扫描区域结束角度。
 *\retval E_OK 表示成功。
 */
e_int32 sj_config(scan_job sj, e_uint32 speed_h, const e_float64 start_angle_h,
		const e_float64 end_angle_h, e_uint32 speed_v, e_float64 resolution_v,
		const e_float64 active_sector_start_angles,
		const e_float64 active_sector_stop_angles) {
	e_int32 ret;
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);

	e_assert(active_sector_start_angles<=90 && active_sector_start_angles>=-45 &&
				active_sector_stop_angles<=90 && active_sector_stop_angles>=-45 &&
				active_sector_stop_angles > active_sector_start_angles, E_ERROR_INVALID_PARAMETER);

	sj->start_angle_v[0] = active_sector_start_angles + 90;
	sj->end_angle_v[0] = active_sector_stop_angles + 90;

	//记录配置
	sj->start_angle_h = start_angle_h;
	sj->end_angle_h = end_angle_h;
	sj->speed_v = speed_v;
	sj->resolution_v = resolution_v;
	sj->speed_h = speed_h;

	//挖个坑,先把第二个角度记下
	sj->start_angle_v[1] = 360 - sj->end_angle_v[0];
	sj->end_angle_v[1] = 360 - sj->start_angle_v[0];

	if (sj->end_angle_v[0] >= 180) { //限制了,垂直角度为 -45~+90 度之间
		sj->end_angle_v[0] = 180;
		sj->start_angle_v[1] = 180 + sj->resolution_v;
	}

	//只会用到0号
	if (sj->start_angle_h < 180 && sj->end_angle_h <= 180) //都在左边
			{
		//都在左边时，此种情况下，控制板的水平角度不用做换算，保持不变
		sj->active_sectors.left = 1;
		sj->active_sectors.right = 0;
	} //会用到0,1号
	else if (sj->start_angle_h < 180 && sj->end_angle_h > 180) //左右皆有
			{
		sj->end_angle_h = sj->start_angle_h + 180.0;
		sj->active_sectors.left = 1;
		sj->active_sectors.right = 1;
	} //只用到2号
	else if (sj->start_angle_h >= 180 && sj->end_angle_h > 180) //都在右边
			{
		sj->start_angle_v[0] = sj->start_angle_v[1];
		sj->end_angle_v[0] = sj->end_angle_v[1];
		sj->start_angle_h = sj->start_angle_h - 180;
		sj->end_angle_h = sj->end_angle_h - 180;
		sj->active_sectors.left = 0;
		sj->active_sectors.right = 1;
	} else {
		DMSG((STDOUT,"cal_h_angle EXTRAM FAILT!\r\n"));
		return E_ERROR;
	}

	sj->height = (sj->end_angle_v[0] - sj->start_angle_v[0])
			/ sj->resolution_v + 1;

	sj->width = ANGLE_TO_STEP( sj->end_angle_h - sj->start_angle_h )
			* PULSE_SPEED_TO_STEP_TIME(sj->speed_h) / 1E6 * sj->speed_v
			* (sj->active_sectors.left + sj->active_sectors.right) + 1;

	sj->pre_scan_angle =
			STEP_TO_ANGLE(PRE_SCAN_WAIT_TIME / PULSE_SPEED_TO_STEP_TIME(sj->speed_h));

	print_config(sj);

	//提交配置到控制板
	ret = hl_turntable_config(&sj->control, sj->speed_h, sj->start_angle_h,
								sj->end_angle_h + sj->pre_scan_angle);
	e_assert(ret>0, ret);

	return E_OK;
}

/**
 *\brief 设置点云数据存储目录,灰度图存储目录。  
 *\param scan_job 定义了扫描任务。 
 *\param ptDir 定义了点云数据存储目录。 
 *\param grayDir 定义了灰度图存储目录。
 *\retval E_OK 表示成功。
 */
e_int32 sj_set_data_file(scan_job sj, char* ptDir, char *grayDir) {
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);
	strncpy(sj->data_file, ptDir, sizeof(sj->data_file));
	strncpy(sj->gray_dir, grayDir, sizeof(sj->gray_dir));
	return E_OK;
}

static e_int32 sj_create_writer(scan_job sj) {
	e_int32 ret;
	system_time_t sys_time;
	GetLocalTime(&sys_time);
	sprintf(sj->data_file, "%d-%d-%d-%d-%d-%d.pcd", sys_time.year,
			sys_time.month, sys_time.day, sys_time.hour, sys_time.minute,
			sys_time.second);
	sprintf(sj->gray_dir, "%d-%d-%d-%d-%d-%d.jpg", sys_time.year,
			sys_time.month, sys_time.day, sys_time.hour, sys_time.minute,
			sys_time.second);
	char *files[] = { sj->data_file, sj->gray_dir, "/tmp/test.sprite" };
	sj->writer = dm_alloc(files, 3, sj->width, sj->height,
							sj->active_sectors.right ? E_DWRITE : E_WRITE);
	e_assert(ret>0, ret);
	return E_OK;
}

static e_uint16 intensity_filter(e_uint32 echo) {
//	if (echo < 70 || echo > 700) //反射强度过滤
//							{
//				echo = 0;
//	}
	return echo;
}

static e_uint8 gray_filter(e_uint32 echo) {
	return (echo - 70) * 255.0 / 300.0;
	return echo * 255.0 * 2 / 600.0;
}

static e_int32 one_slip(scan_job sj, scan_data_t * pdata, e_int32 data_idx,
		e_uint32 data_num, e_int32 type) {
//	unsigned int pnt_idx = 0;
	unsigned int gray_idx = sj->slip_idx;
	point_gray_t* pgray;
	point_polar_t *ppoint;

	if (sj->slip_idx >= sj->width) {
		DMSG((STDOUT,"sj->slip_idx > sj->width,忽略多余数据\n"));
		return E_OK;
	}

	if (type && sj->active_sectors.left) {
		gray_idx += sj->width / 2; //有左区,写右区时要跳过左区
	}

	ppoint = (point_polar_t*) sj->points_xyz->mem;
	pgray = (point_gray_t*) sj->points_gray->mem;

//	if (data_num > sj->height) //点数个数多一个?不可能
//			{
//		DMSG((STDOUT,"#ERROR# 点数个数多一个?不可能 data_num[%u] sj->height[%u]",(unsigned int)data_num,(unsigned int)sj->height));
//		data_num = sj->height; //舍掉的刚好是最后一个点
//	}
//	else if (data_num < sj->height) { //点数个数不够?不可能
//		DMSG((STDOUT,"#ERROR# 点数个数少一个?不可能 [[data_num <sj->height]] why?\n"));
//	}

	if (e_check(data_num != sj->height && data_num+1 != sj->height, "#ERROR# 点数个数与要求的不一致!\n"))
	{
		DMSG((STDOUT,"#ERROR# 点数个数多一个?不可能 data_num[%u] sj->height[%u]\n",
				(unsigned int)data_num,(unsigned int)sj->height));
		return E_ERROR;
	}
//	if(data_num+1 == sj->height) data_num = sj->height;

	if (type)
	{
		for (int k = data_num - 1; k >= 0; --k) {
			ppoint->distance = pdata->range_values[data_idx + k];
			ppoint->angle_h = pdata->h_angle;
			ppoint->angle_v = pdata->scan_angles[data_idx + k];
			ppoint->intensity = intensity_filter(pdata->echo_values[data_idx + k]);
			pgray->gray = gray_filter(pdata->echo_values[data_idx + k]);
//			if (pnt_idx >= sj->height) //点数个数限制
//					{
//				DMSG((STDOUT,"#ERROR# FOUD pnt_idx >= sj->height\r\n"));
//				break;
//			}
//			pnt_idx++;
			if (ppoint->distance <= 0.0000005) {
				DMSG((STDOUT,"I"));
			}
			ppoint++;
			pgray++;
		} // end for
	}
	else
	{
		for (unsigned int k = 0; k < data_num; ++k) {
			ppoint->distance = pdata->range_values[data_idx + k];
			ppoint->angle_h = pdata->h_angle;
			ppoint->angle_v = pdata->scan_angles[data_idx + k];
			ppoint->intensity = intensity_filter(pdata->echo_values[data_idx + k]);
			pgray->gray = gray_filter(pdata->echo_values[data_idx + k]);
//			if (pnt_idx >= sj->height) //点数个数限制
//					{
//				DMSG((STDOUT,"#ERROR# FOUD pnt_idx >= sj->height\r\n"));
//				break;
//			}
//			pnt_idx++;
			if (ppoint->distance <= 0.0000005) {
				DMSG((STDOUT,"|"));
			}
			ppoint++;
			pgray++;
		} // end for
	}
//	if (pnt_idx < sj->height) //点数个数不够?
//			{
//		DMSG((STDOUT,"#ERROR# type:%d,data_num:%u,FOUD pnt_idx[%u] < sj->height[%u]\r\n",
//				type,(unsigned int)data_num,pnt_idx,(unsigned int)sj->height));
//	}

//if (!bPreScan)
	{
		dm_append_points(sj->writer, sj->points_xyz, sj->height, type);
		dm_append_points(sj->writer, sj->points_gray, sj->height, type);
	}

	return E_OK;
}

/************************************************************************/
/*         根据当前水平角度范围和垂直角度范围，得到每一圈实际扫描数据和灰度图        */
/************************************************************************/
e_int32 sj_filter_data(scan_job sj, scan_data_t * pdata) {
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);
	int didx = 0;

	if (sj->active_sectors.left) {
		one_slip(sj, pdata, pdata->data_offsets[didx], pdata->num_values[didx], 0);
		didx++;
	}

	if (sj->active_sectors.right) {
		one_slip(sj, pdata, pdata->data_offsets[didx], pdata->num_values[didx], 1);
	}

	sj->slip_idx++;

	return E_OK;
}

int sj_save2image(char *filename, unsigned char *bits, int width, int height,
		int depth) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile; /* target file */
	JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
	int row_stride; /* physical row width in image buffer */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s/n", filename);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = width; /* image width and height, in pixels */
	cinfo.image_height = height;
	cinfo.input_components = 1; /* # of color components per pixel */
	cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, JPEG_QUALITY,
						TRUE /* limit to baseline-JPEG values */);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = width * depth; /* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		//这里我做过修改，由于jpg文件的图像是倒的，所以改了一下读的顺序
		row_pointer[0] = &bits[cinfo.next_scanline * row_stride];
		//row_pointer[0] = & bits[(cinfo.image_height - cinfo.next_scanline - 1) * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
	return 0;
}

static int sj_save2image_rotation(char *filename, unsigned char *bits, int width,
		int height) {
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile; /* target file */
	JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
	int row_stride; /* physical row width in image buffer */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s/n", filename);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = height; /* image width and height, in pixels */
	cinfo.image_height = width;
	cinfo.input_components = 1; /* # of color components per pixel */
	cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, JPEG_QUALITY,
						TRUE /* limit to baseline-JPEG values */);

	jpeg_start_compress(&cinfo, TRUE);

	row_stride = height; /* JSAMPLEs per row in image_buffer */

	unsigned char *row = (unsigned char *) malloc(row_stride);
	row_pointer[0] = row;
	while (cinfo.next_scanline < cinfo.image_height) {
		for (int i = 0; i < row_stride; i++) { //get one row
			row[i] = bits[cinfo.next_scanline + i * width];
		}
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	free(row);

	jpeg_finish_compress(&cinfo);
	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
	return 0;
}

