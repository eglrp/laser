#include "hd_laser_scan.h"
#include <hd_timer_api.h>
#include <hd_utils.h>

enum
{
	STATE_NONE = 0, STATE_INIT = 1, STATE_START = 2, STATE_STOP = 3,
};
static e_int32 sj_clean(scan_job_t *sj);
static void write_pool_data_routine(void* data)
{
	e_int32 ret;
	scan_job_t *sj = (scan_job_t *) data;
	scan_data_t sdata;
	DMSG((STDOUT,"scan job:write_data_routine start.\r\n"));
	//TODO:更详细的异常处理
	while (sj->state == STATE_START)
	{
		//1先读取水平方向角度
		sdata.h_angle = hl_turntable_get_angle(&sj->control);
		DMSG((STDOUT,"WRITE TO FILE:",sdata.h_angle));
		//转台是否已经停止转动了
		ret = hl_turntable_is_stopped(&sj->control);
		if (ret)
		{
			break;
		}
		ret = sld_get_measurements(sj->sick, sdata.range_values,
				sdata.echo_values, sdata.scan_angles, sdata.num_values,
				sdata.sector_ids, sdata.data_offsets, NULL, NULL, NULL, NULL,
				NULL);
		if (ret <= 0)
		{
			DMSG((STDOUT,"sld_get_measurements ERROR: RETRY.\r\n"));
			Delay(10);
			continue;
		}
		ret = pool_write(&sj->pool, &sdata);
		if (ret <= 0)
		{
			DMSG((STDOUT,"pool_write ERROR: RETRY.\r\n"));
			Delay(10);
			continue;
		}
	}
	DMSG((STDOUT,"scan job:write_data_routine done."));
	sj_clean(sj);
}

static void read_pool_data_routine(void* data)
{
	e_int32 ret;
	scan_job_t *sj = (scan_job_t *) data;
	scan_data_t sdata;
	DMSG((STDOUT,"scan job:read_data_routine start.\r\n"));
	//TODO:更详细的异常处理
	while (sj->state == STATE_START)
	{
		ret = pool_read(&sj->pool, &sdata);
		if (ret <= 0)
		{
			DMSG((STDOUT,"pool_write ERROR: RETRY.\r\n"));
			Delay(10);
			continue;
		}
		//TODO:写入文件
		DMSG(
				(STDOUT,"read_data_routine write data at angle: %f.\r\n",(float)sdata.h_angle));
	}
	DMSG((STDOUT,"scan job:write_data_routine done.\r\n"));
	sj_clean(sj);
}

/************************************************************************/
/* 与扫描仪建立连接，并初始化扫描参数信息                                      */
/************************************************************************/
e_int32 sj_init(scan_job_t* sj)
{
	e_int32 ret;
	memset(sj, 0, sizeof(scan_job_t));
	ret = hl_open(&sj->control, NULL, NULL);
	e_assert(ret>0, ret);
	ret = sld_create(&sj->sick, NULL, NULL);
	e_assert(ret>0, ret);
	pool_init(&sj->pool);
	sj->state = 1;
	return E_OK;
}

/************************************************************************/
/* 与扫描仪断开连接                                                        */
/************************************************************************/
e_int32 sj_destroy(scan_job_t *sj)
{
	hl_close(&sj->control);
	pool_destroy(&sj->pool);
	sld_release(&sj->sick);
	sj->state = 0;
	return E_OK;
}

/************************************************************************
 * 开始扫描,此时开启读写两个线程
 * 读线程负责读取扫描数据，并写入共享队列
 * 写线程负责读取共享队列数据，并写入到文件
 ************************************************************************/
e_int32 sj_start(scan_job_t *sj)
{
	e_int32 ret;
	DMSG((STDOUT, "scan job routine starting...\r\n"));
	if (sj->state != STATE_INIT)
		return E_OK;
	sj->state = STATE_START;
	//等待扫描仪就绪
	ret = sld_initialize(sj->sick);
	if (ret <= 0)
	{
		DMSG((STDOUT, "hd laser sick start failed!\r\n"));
		return E_ERROR;
	}

	ret = hl_turntable_prepare(&sj->control);
	if (ret <= 0)
	{
		DMSG((STDOUT, "hd laser control prepare failed!\r\n"));
		return E_ERROR;
	}

	ret = sld_set_global_params_and_scan_areas(sj->sick, sj->speed,
			sj->resolution, sj->start_angle_v, sj->end_angle_v,
			sj->active_sectors_num);
	e_assert(ret>0, ret);

	ret = hl_turntable_start(&sj->control);
	if (ret <= 0)
	{
		DMSG((STDOUT, "hd laser control start failed!\r\n"));
		return E_ERROR;
	}
	Delay(300); //等待控制板开始转动300ms

	DMSG((STDOUT, "scan job routine starting write routine...\r\n"));
	ret = createthread("hd_scan_job_write", (thread_func) &write_pool_data_routine,
			sj, NULL, &sj->thread_write);
	if (ret <= 0)
	{
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(sj->thread_write);
	if (ret <= 0)
	{
		DMSG((STDOUT, "resumethread failed!\r\n"));
		return E_ERROR;
	}
	DMSG((STDOUT, "scan job routine starting read routine...\r\n"));
	ret = createthread("hd_scan_job_read", (thread_func) &read_pool_data_routine, sj,
			NULL, &sj->thread_read);
	if (ret <= 0)
	{
		DMSG((STDOUT, "createhread failed!\r\n"));
		return E_ERROR;
	}
	ret = resumethread(sj->thread_write);
	if (ret <= 0)
	{
		DMSG((STDOUT, "resumethread failed!\r\n"));
		return E_ERROR;
	}
	DMSG((STDOUT, "scan job routine start done.\r\n"));
	return E_OK;
}

/************************************************************************
 * 停止扫描                                                              *
 ************************************************************************/
e_int32 sj_stop(scan_job_t *sj)
{
	DMSG((STDOUT, "scan job routine stop...\r\n"));
	if (sj->state == STATE_START)
	{
		sj->state = STATE_STOP;
	}
	while (sj->state != STATE_NONE)
		Delay(1000); //等待子线程止
	return E_OK;
}

static e_int32 sj_clean(scan_job_t *sj)
{
	DMSG((STDOUT, "scan job routine clean...\r\n"));
	if (sj->state == STATE_STOP)
	{
		sld_initialize(sj->sick);
		sj->state = STATE_NONE;
	}
	return E_OK;
}

/************************************************************************/
/*         根据传入的灰度图水平角度范围换算实际控制台转动水平角度                                                           */
/************************************************************************/
static e_bool cal_h_angle(e_float64* start, e_float64* end)
{
	if ((*start) < 180 && (*end) <= 180) //都在左边
	{
		//都在左边时，此种情况下，控制板的水平角度不用做换算，保持不变
		return false;
	}
	else if ((*start) < 180 && (*end) > 180) //左右皆有
	{
		(*start) = 0.0;
		(*end) = 180;
		return true;
	}
	else if ((*start) >= 180 && (*end) > 180) //都在右边
	{
		(*start) = (*start) - 180;
		(*end) = (*end) - 180;
		return false;
	}
	else
	{
		DMSG((STDOUT,"cal_h_angle EXTRAM FAILT!\r\n"));
		return false;
	}
}

/************************************************************************/
/* 设置扫描区域                                                                   */
/************************************************************************/
e_int32 sj_config(scan_job_t *sj, e_uint32 speed, e_float64 resolution,
		const e_float64 start_angle_h, const e_float64 end_angle_h,
		const e_float64 * active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_int32 num_active_sectors)
{
	e_int32 ret;
	e_bool is_two_sector_h = false;
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);

	//TODO:为什么差90度
	sj->start_angle_v[0] = active_sector_start_angles[0] + 90;
	sj->end_angle_v[0] = active_sector_stop_angles[0] + 90;

	//记录配置
	sj->start_angle_h = start_angle_h;
	sj->end_angle_h = end_angle_h;
	is_two_sector_h = cal_h_angle(&sj->start_angle_h, &sj->end_angle_h);
	sj->speed = speed;
	sj->resolution = resolution;

	//提交配置到控制板
	ret = hl_turntable_config(&sj->control, speed, sj->start_angle_h,
			sj->start_angle_h);
	e_assert(ret>0, ret);

	if (is_two_sector_h)
	{
		sj->start_angle_v[1] =
				(sj->end_angle_v[0] >= 180) ?
						(360 - sj->end_angle_v[0] + resolution) :
						(360 - sj->end_angle_v[0]);

		sj->end_angle_v[1] =
				(sj->start_angle_v[0] == 0) ?
						(360 - resolution) : (360 - sj->start_angle_v[0]); //角度最大是不可以超过360的
		sj->active_sectors_num = 2;
	}
	else
	{
		sj->active_sectors_num = 1;
		if (start_angle_h >= 180 && end_angle_h > 180)
		// 如果只扫一边，且是右边的话，垂直区域应该是后半圈，如果是左边的话，则是前半圈
		{
			sj->start_angle_v[0] =
					(sj->end_angle_v[0] >= 180) ?
							(360 - sj->end_angle_v[0] + resolution) :
							(360 - sj->end_angle_v[0]);
			sj->end_angle_v[0] =
					(sj->start_angle_v[0] == 0) ?
							(360 - resolution) : (360 - sj->start_angle_v[0]); //角度最大是不可以超过360的

		}
	}
	return E_OK;
}

/************************************************************************/
/* 设置点云数据存储目录,灰度图存储目录                      	                */
/************************************************************************/
e_int32 sj_set_data_dir(scan_job_t *sj, char* ptDir, e_uint8 *grayDir)
{
	e_int32 ret;
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);
	return E_OK;
}

/************************************************************************/
/*         根据当前水平角度范围和垂直角度范围，得到每一圈实际扫描数据和灰度图        */
/************************************************************************/
e_int32 sj_filter_data(scan_job_t *sj)
{
	e_int32 ret;
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);
}

