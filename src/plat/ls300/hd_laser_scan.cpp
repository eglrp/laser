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

extern "C" {
#include <jpeg/jpeglib.h>
}

#include <arch/hd_timer_api.h>
#include <comm/hd_utils.h>

#define JPEG_QUALITY 50     //它的大小决定jpg的质量好坏

enum
{
	STATE_NONE = 0, STATE_INIT = 1, STATE_START = 2, STATE_STOP = 3,
};
static e_int32 sj_clean(scan_job_t *sj);
static e_int32 sj_create_hlswriter(scan_job_t *sj);
static void scan_done(scan_job_t *sj);
int sj_save2image(char *filename, unsigned char *bits, int width, int height, int depth);

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
		DMSG((STDOUT,"WRITE TO FILE: %f",sdata.h_angle));
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
				(STDOUT,"read_data_routine write data at angle: %f.\r\n", (float)sdata.h_angle));
		sj_filter_data(sj, &sdata);
	}
	scan_done(sj);
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

static void pre_start(scan_job_t *sj)
{
	//保证每半圈的数据都是相等的，需要对某一个半圈过滤掉一个点
	sj->height =
			(sj->end_angle_v[0] >= 180) ?
					((sj->end_angle_v[0] - sj->start_angle_v[0])
							/ sj->resolution_v - 1) :
					((sj->end_angle_v[0] - sj->start_angle_v[0])
							/ sj->resolution_v);
	sj->width = ANGLE_TO_STEP( sj->end_angle_h - sj->start_angle_h )
			* sj->speed_h * sj->speed_v * sj->height
			* (sj->active_sectors.left + sj->active_sectors.right);

	//创建数据文件
	sj_create_hlswriter(sj);
	//创建缓冲区
	sj->points = (hd::HLSPointStru*) malloc(
			sj->height * sizeof(hd::HLSPointStru));
	//创建色彩缓冲
	sj->gray = (e_uint8*) malloc(sj->width * sizeof(e_uint8));

	sj->slip_idx = 0;
}

static void scan_done(scan_job_t *sj)
{
	sj->hlsWrite.close(sj->height * sj->slip_idx + 1, sj->height, sj->slip_idx); //写入行列号
	sj_save2image(sj->gray_dir,sj->gray,sj->width,sj->height,sizeof(e_uint8)); //存储灰度图
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

	ret = sld_set_global_params_and_scan_areas(sj->sick, sj->speed_v,
			sj->resolution_v, sj->start_angle_v, sj->end_angle_v,
			sj->active_sectors.left + sj->active_sectors.right);
	e_assert(ret>0, ret);

	//开始真正数据采集前的准备工作
	ret = hl_turntable_prepare(&sj->control);
	if (ret <= 0)
	{
		DMSG((STDOUT, "hd laser control prepared failed!\r\n"));
		return E_ERROR;
	}

	pre_start(sj);

	ret = hl_turntable_start(&sj->control);
	if (ret <= 0)
	{
		DMSG((STDOUT, "hd laser control start failed!\r\n"));
		return E_ERROR;
	}
	Delay(300); //等待控制板开始转动300ms

	DMSG((STDOUT, "scan job routine starting write routine...\r\n"));
	ret = createthread("hd_scan_job_write",
			(thread_func) &write_pool_data_routine, sj, NULL,
			&sj->thread_write);
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
	ret = createthread("hd_scan_job_read",
			(thread_func) &read_pool_data_routine, sj, NULL, &sj->thread_read);
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
		sld_uninitialize(sj->sick);
		sj->state = STATE_NONE;

		killthread(sj->thread_read);
		killthread(sj->thread_write);
	}
	return E_OK;
}

/************************************************************************/
/* 设置扫描区域                                                           */
/************************************************************************/
e_int32 sj_config(scan_job_t *sj, e_uint32 speed_h,
		const e_float64 start_angle_h, const e_float64 end_angle_h,
		e_uint32 speed_v, e_float64 resolution_v,
		const e_float64 active_sector_start_angles,
		const e_float64 active_sector_stop_angles)
{
	e_int32 ret;
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);

	sj->start_angle_v[0] = active_sector_start_angles + 90;
	sj->end_angle_v[0] = active_sector_stop_angles + 90;

	//记录配置
	sj->start_angle_h = start_angle_h;
	sj->end_angle_h = end_angle_h;
	sj->speed_v = speed_v;
	sj->resolution_v = resolution_v;
	sj->speed_h = speed_h;

	//挖个坑,先把第二个角度记下
	sj->start_angle_v[1] =
			(sj->end_angle_v[0] >= 180) ?
					(360 - sj->end_angle_v[0] + sj->resolution_v) :
					(360 - sj->end_angle_v[0]);

	sj->end_angle_v[1] =
			(sj->start_angle_v[0] == 0) ?
					(360 - sj->resolution_v) : (360 - sj->start_angle_v[0]);
	//角度最大是不可以超过360的

	//只会用到0号
	if (sj->start_angle_h < 180 && sj->end_angle_h <= 180) //都在左边
	{
		//都在左边时，此种情况下，控制板的水平角度不用做换算，保持不变
		sj->active_sectors.left = 1;
		sj->active_sectors.right = 0;
	} //会用到0,1号
	else if (sj->start_angle_h < 180 && sj->end_angle_h > 180) //左右皆有
	{
		sj->start_angle_h = 0.0;
		sj->end_angle_h = 180;
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
	}
	else
	{
		DMSG((STDOUT,"cal_h_angle EXTRAM FAILT!\r\n"));
		return E_ERROR;
	}

	//提交配置到控制板
	ret = hl_turntable_config(&sj->control, sj->speed_h, sj->start_angle_h,
			sj->start_angle_h);
	e_assert(ret>0, ret);

	return E_OK;
}

/************************************************************************/
/* 设置点云数据存储目录,灰度图存储目录                      	                */
/************************************************************************/
e_int32 sj_set_data_dir(scan_job_t *sj, char* ptDir, char *grayDir)
{
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);
	strncpy(sj->data_dir, ptDir, sizeof(sj->data_dir));
	strncpy(sj->gray_dir, grayDir, sizeof(sj->gray_dir));
	return E_OK;
}

static void create_guid(int *num1, short *num2, short *num3, long long* num4)
{
	srand(GetTickCount());
	(*num1) = rand();
	(*num2) = rand() & 0xFFFF;
	(*num3) = rand() & 0xFFFF;
	(*num4) = (((long long) rand()) & 0xFFFF) << 16 | (rand() & 0xFFFF);
}

static e_int32 sj_create_hlswriter(scan_job_t *sj)
{
	hd::HLSheader hlsHeader;
	strcpy(hlsHeader.file_signature, "HLSF");
	int num1 = 0;
	short num2 = 0;
	short num3 = 0;
	long long num4 = 0;
	create_guid(&num1, &num2, &num3, &num4); //仍然挂的话就把这提出到线程外部
	hlsHeader.project_ID_GUID_data_1 = num1;
	hlsHeader.project_ID_GUID_data_2 = num2;
	hlsHeader.project_ID_GUID_data_3 = num3;
	memcpy(hlsHeader.project_ID_GUID_data_4, &num4, sizeof(num4));
	//
	hlsHeader.file_source_id = 0;
	hlsHeader.version_major = 1;
	hlsHeader.version_minor = 0;
	strcpy(hlsHeader.system_identifier, "HD 3LS 001");
	strcpy(hlsHeader.generating_software, "HD-3LS-SCAN");
	//在文件头中写入倾角信息
	hlsHeader.yaw = 0.0f; //扫描仪航向
	hlsHeader.pitch = 0.0f; //dY;//扫描仪前后翻滚角
	hlsHeader.roll = 0.0f; //dX;//扫描仪左右翻滚角

	system_time_t sys_time;
	GetLocalTime(&sys_time);
	hlsHeader.file_creation_day = sys_time.day;
	hlsHeader.file_creation_year = sys_time.year;
	sj->hlsWrite.create(sj->data_dir, &hlsHeader);
	return E_OK;
}

static e_int32 one_slip(scan_job_t *sj, scan_data_t * pdata, e_int32 data_idx,
		e_int32 data_num, e_int32 type)
{
	unsigned int pnt_idx = 0;
	unsigned int gray_idx = sj->slip_idx;
	e_uint8* pgray;
	hd::HLSPointStru *ppoint;

	if (type && sj->active_sectors.left)
	{
		gray_idx += sj->width / 2;//有左区,写右区时要跳过左区
	}

	memset(sj->points, 0, sizeof(sj->points)); //清空缓存
	ppoint = sj->points;
	pgray = &sj->gray[gray_idx * sj->height * sizeof(e_uint8) + pnt_idx];

	for (int k = data_num - 1; k >= 0; --k)
	{
		ppoint->x = pdata->range_values[data_idx + k];
		ppoint->y = pdata->h_angle;
		ppoint->z = pdata->scan_angles[data_idx + k] - 0.5 * k / (data_num - 1);

		if (pdata->echo_values[data_idx + k] >= 70
				&& pdata->echo_values[data_idx + k] <= 700) //反射强度过滤
		{
			ppoint->intensity = pdata->echo_values[data_idx + k];
		}

		if (pnt_idx >= sj->height) //点数个数限制
		{
			DMSG((STDOUT,"FOUD pnt_idx >= sj->height\r\n"));
			break;
		}

		(*pgray) = pdata->echo_values[data_idx + k] & 0xFF;
		pnt_idx++;
		ppoint++;
		pgray++;
	} // end for

	if (pnt_idx < sj->height - 1) //点数个数不够?
	{
		DMSG((STDOUT,"pnt_idx < sj->height-1\r\n"));
	}

	//if (!bPreScan)
	{
		sj->hlsWrite.write_point(sj->points, sj->height, type);
	}

	return E_OK;
}

/************************************************************************/
/*         根据当前水平角度范围和垂直角度范围，得到每一圈实际扫描数据和灰度图        */
/************************************************************************/
e_int32 sj_filter_data(scan_job_t *sj, scan_data_t * pdata)
{
	e_assert(sj&&sj->state, E_ERROR_INVALID_HANDLER);

	//填充左边的，获取一圈数据进行填充时，首先作如下判断
	// 1 当前水平角度是否在扫描水平角度左边范围，如果是则到2，否则不做填充
	// 2 判断一圈数据中那些是左边扫描范围的数据，如果是则填充，否则放弃，
	//填充右边时，做出同上的判断，看那些数据需要，那些数据不需要
	//----------------------------------------begin-----------------------------------
	int data_idx = pdata->data_offsets[0];
	int data_num = pdata->num_values[0];

	if (sj->active_sectors.left)
	{
		one_slip(sj, pdata, data_idx, data_num, 0);
		data_idx = pdata->data_offsets[1];
		data_num = pdata->num_values[1];
	}

	if (sj->active_sectors.right)
	{
		one_slip(sj, pdata, data_idx, data_num, 1);
	}

	sj->slip_idx++;

	return E_OK;
}


int sj_save2image(char *filename, unsigned char *bits, int width, int height, int depth)
{
	struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE * outfile;                 /* target file */
    JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
    int     row_stride;             /* physical row width in image buffer */

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    if ((outfile = fopen(filename, "wb")) == NULL) {
            fprintf(stderr, "can't open %s/n", filename);
            return -1;
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;      /* image width and height, in pixels */
    cinfo.image_height = height;
    cinfo.input_components = 3;         /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB;         /* colorspace of input image */

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE /* limit to baseline-JPEG values */);

    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * depth; /* JSAMPLEs per row in image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) {
            //这里我做过修改，由于jpg文件的图像是倒的，所以改了一下读的顺序
            row_pointer[0] = & bits[cinfo.next_scanline * row_stride];
       // row_pointer[0] = & bits[(cinfo.image_height - cinfo.next_scanline - 1) * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);

    jpeg_destroy_compress(&cinfo);
	return 0;
}

