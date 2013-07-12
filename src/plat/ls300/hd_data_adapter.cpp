/*
 * =====================================================================================
 *
 *       Filename:  hd_data_adapter.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年06月19日 15时03分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Mei Kang (), meikang9527@163.com
 *        Company:  College of Information Engineering of CDUT
 *
 *        modify:
 *        			Joy.you 20120628  	接口重构,PCL格式重构
 *        			Joy.you 20120629  	添加JPG格式支持
 *        			Joy.you 20120629  	添加GIF格式支持
 *
 *
 * =====================================================================================
 */

#include <ls300/hd_data_adapter.h>
#include <arch/hd_file_api.h>
#include <comm/hd_utils.h>
#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch/hd_pipe_api.h>
#include <ls300/hd_connect.h>
#include <netinet/in.h>

extern "C" {
#include <jpeg/jpeglib.h>
#define JPEG_QUALITY 100

#include <gif/gif_api.h>
} ////////////////////////////////////////////////////////////////////////
//底层开发
static e_int32 register_file_format(file_op_t* op, int i); //注册文件读写接口

//PCL
static e_int32 inter_pcl_open(file_ptr_t *file);
static e_int32 inter_pcl_combine(file_ptr_t *file1, file_ptr_t *file2);
static e_int32 inter_pcl_close(file_ptr_t *file);
static e_int32 inter_pcl_write_point(file_ptr_t *file, int x, int y, point_t* point);
static e_int32 inter_pcl_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* point);
static e_int32 inter_pcl_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point);
static e_int32 inter_pcl_append_points(file_ptr_t *file, point_t* point, int pt_num);
static e_int32 inter_pcl_append_row(file_ptr_t *file, point_t* point);
static e_int32 inter_pcl_append_column(file_ptr_t *file, point_t* point);
static e_int32 inter_pcl_read_points(file_ptr_t *file, point_t* point, int buf_len);
static e_int32 inter_pcl_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf);
static e_int32 inter_pcl_read_column(file_ptr_t *file, e_uint32 column_idx, point_t* buf);

//jpeg
static e_int32 inter_jpg_open(file_ptr_t *file);
static e_int32 inter_jpg_combine(file_ptr_t *file1, file_ptr_t *file2);
static e_int32 inter_jpg_close(file_ptr_t *file);
static e_int32 inter_jpg_write_point(file_ptr_t *file, int x, int y, point_t* point);
static e_int32 inter_jpg_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* point);
static e_int32 inter_jpg_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point);
static e_int32 inter_jpg_append_points(file_ptr_t *file, point_t* point, int pt_num);
static e_int32 inter_jpg_append_row(file_ptr_t *file, point_t* point);
static e_int32 inter_jpg_append_column(file_ptr_t *file, point_t* point);
static e_int32 inter_jpg_read_points(file_ptr_t *file, point_t* point, int buf_len);
static e_int32 inter_jpg_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf);
static e_int32 inter_jpg_read_column(file_ptr_t *file, e_uint32 column_idx, point_t* buf);

//gif
static e_int32 inter_gif_open(file_ptr_t *file);
static e_int32 inter_gif_combine(file_ptr_t *file1, file_ptr_t *file2);
static e_int32 inter_gif_close(file_ptr_t *file);
static e_int32 inter_gif_write_point(file_ptr_t *file, int x, int y, point_t* point);
static e_int32 inter_gif_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* point);
static e_int32 inter_gif_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point);
static e_int32 inter_gif_append_points(file_ptr_t *file, point_t* point, int pt_num);
static e_int32 inter_gif_append_row(file_ptr_t *file, point_t* point);
static e_int32 inter_gif_append_column(file_ptr_t *file, point_t* point);
static e_int32 inter_gif_read_points(file_ptr_t *file, point_t* point, int buf_len);
static e_int32 inter_gif_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf);
static e_int32 inter_gif_read_column(file_ptr_t *file, e_uint32 column_idx, point_t* buf);

//sprite
static e_int32 inter_sprite_open(file_ptr_t *file);
static e_int32 inter_sprite_combine(file_ptr_t *file1, file_ptr_t *file2);
static e_int32 inter_sprite_close(file_ptr_t *file);
static e_int32 inter_sprite_write_point(file_ptr_t *file, int x, int y, point_t* point);
static e_int32 inter_sprite_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* point);
static e_int32 inter_sprite_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point);
static e_int32 inter_sprite_append_points(file_ptr_t *file, point_t* point, int pt_num);
static e_int32 inter_sprite_append_row(file_ptr_t *file, point_t* point);
static e_int32 inter_sprite_append_column(file_ptr_t *file, point_t* point);
static e_int32 inter_sprite_read_points(file_ptr_t *file, point_t* point, int buf_len);
static e_int32 inter_sprite_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf);
static e_int32 inter_sprite_read_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* buf);
static e_int32 inter_sprite_on_data_change(file_ptr_t *file1, file_ptr_t *file2);

//open和close是必须有的接口
file_adapter_t file_adpt[] = {
	{
		file_suffix : "pcd",
		pnt_type: PNT_TYPE_XYZ,
		op : {
			open:inter_pcl_open,
			combine:inter_pcl_combine,
			on_data_change:NULL,
			close:inter_pcl_close,
			write_point:inter_pcl_write_point,
			write_row:inter_pcl_write_row,
			write_column:inter_pcl_write_column,
			append_points:inter_pcl_append_points,
			append_row:inter_pcl_append_row,
			append_column:inter_pcl_append_column,
			read_points:inter_pcl_read_points,
			read_row:inter_pcl_read_row,
			read_column:inter_pcl_read_column
		}
	},
	{
		file_suffix : "jpg",
		pnt_type: PNT_TYPE_GRAY,
		op : {
			open:inter_jpg_open,
			combine:inter_jpg_combine,
			on_data_change:NULL,
			close:inter_jpg_close,
			write_point:inter_jpg_write_point,
			write_row:inter_jpg_write_row,
			write_column:inter_jpg_write_column,
			append_points:inter_jpg_append_points,
			append_row:inter_jpg_append_row,
			append_column:inter_jpg_append_column,
			read_points:inter_jpg_read_points,
			read_row:inter_jpg_read_row,
			read_column:inter_jpg_read_column
		}
	},
	{
		file_suffix : "gif",
		pnt_type: PNT_TYPE_GRAY,
		op : {
			open:inter_gif_open,
			combine:inter_gif_combine,
			on_data_change:NULL,
			close:inter_gif_close,
			write_point:inter_gif_write_point,
			write_row:inter_gif_write_row,
			write_column:inter_gif_write_column,
			append_points:inter_gif_append_points,
			append_row:inter_gif_append_row,
			append_column:inter_gif_append_column,
			read_points:inter_gif_read_points,
			read_row:inter_gif_read_row,
			read_column:inter_gif_read_column
		}
	},
	{
		file_suffix : "sprite",
		pnt_type: PNT_TYPE_GRAY,
		op : {
			open:inter_sprite_open,
			combine:inter_sprite_combine,
			on_data_change:inter_sprite_on_data_change,
			close:inter_sprite_close,
			write_point:inter_sprite_write_point,
			write_row:inter_sprite_write_row,
			write_column:inter_sprite_write_column,
			append_points:inter_sprite_append_points,
			append_row:inter_sprite_append_row,
			append_column:inter_sprite_append_column,
			read_points:inter_sprite_read_points,
			read_row:inter_sprite_read_row,
			read_column:inter_sprite_read_column
		}
	},
};

const static int FILE_TYPE_NUM = sizeof(file_adpt) / sizeof(file_adapter_t);

e_int32 da_open(data_adapter_t *da, e_uint8 *name, int width, int height, int mode) {
	e_assert(strlen((char*) name) < FILE_NAME_MAX_LENGTH, E_ERROR);

	memset(da, 0, sizeof(data_adapter_t));

	char tmp[FILE_NAME_MAX_LENGTH] = { 0 };
	char* file_type = NULL;
	int num;

	strcpy(tmp, (char*) name);
	strcpy(da->file_info.file_name, (char*) name);

	da->file_info.width = width;
	da->file_info.height = height;
	da->file_info.mode = mode;

	file_type = strtok(tmp, ".");
	file_type = strtok(NULL, ".");

	for (num = 0; num < FILE_TYPE_NUM; num++) {
		int ret = strncmp(file_type, file_adpt[num].file_suffix,
							strlen(file_adpt[num].file_suffix));
		if (!ret) {
			register_file_format(&da->op, num);
			da->file_info.pnt_type = file_adpt[num].pnt_type;
			if (mode == E_READ) {
				da->file.read.info = da->file_info;
				da->op.open(&da->file.read);
				da->file_info = da->file.read.info;
			}
			else if (E_DWRITE == mode) {
				da->file.write.left.info = da->file_info;
				da->file.write.left.is_main = 1;
				da->file.write.right.info = da->file_info;
				strcat(da->file.write.right.info.file_name, ".tmp");
				da->op.open(&da->file.write.left);
				da->op.open(&da->file.write.right);
			}
			else if (E_WRITE == mode) {
				da->file.write.left.info = da->file_info;
				da->file.write.left.is_main = 1;
				da->op.open(&da->file.write.left);
			}
			break;
		}
	}
	if (num == FILE_TYPE_NUM)
		return E_ERROR;

	da->state = 1;
	return E_OK;
}

e_int32 da_close(data_adapter_t *da) {
	e_int32 ret;
	e_assert(da && da->state, E_ERROR_INVALID_HANDLER);
	if (da->file_info.mode == E_READ) {
		ret = da->op.close(&da->file.read);
	} else if (da->file_info.mode == E_DWRITE) {
		if (da->op.combine) {
			ret = da->op.combine(&da->file.write.left, &da->file.write.right);
			e_assert(ret > 0, ret);
		}
		da->file.write.left.info.mode = E_WRITE;
		ret = da->op.close(&da->file.write.right);
		e_assert(ret > 0, ret);
		ret = da->op.close(&da->file.write.left);
	} else if (da->file_info.mode == E_WRITE) {
		ret = da->op.close(&da->file.write.left);
	}
	e_assert(ret > 0, ret);
	da->state = 0;
	return E_OK;
}

e_int32 da_read_points(data_adapter_t *da, point_t* point, int buf_len)
		{
	e_assert(da && da->state && da->op.read_points && da->file_info.mode == E_READ,
				E_ERROR_INVALID_HANDLER);
	da->op.read_points(&da->file.read, point, buf_len);
	return E_OK;
}

e_int32 da_append_points(data_adapter_t *da, point_t* point, int pt_num, int file_right) {
	e_int32 ret;
	e_assert(da && da->state && da->op.append_points && point,
				E_ERROR_INVALID_HANDLER);
	file_ptr_t * file;

	if (!file_right) {
		e_assert(da->file_info.mode==E_WRITE || da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.left;
	}
	else {
		e_assert(da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.right;
	}

	file->modify = 1;
	e_assert(file->cousor + pt_num
				<= file->info.width * file->info.height, E_ERROR);
	ret = da->op.append_points(file, point, pt_num);

	if (da->op.on_data_change)
		da->op.on_data_change(&da->file.write.left, &da->file.write.right);

	return ret;
}

e_int32 da_append_row(data_adapter_t *da, point_t* point, int file_right)
		{
	e_int32 ret;
	e_assert(da && da->state && da->op.append_row && point, E_ERROR_INVALID_HANDLER);
	file_ptr_t * file;

	if (!file_right) {
		e_assert(da->file_info.mode==E_WRITE || da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.left;
	}
	else {
		e_assert(da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.right;
	}

	file->modify = 1;
	e_assert(file->cousor + file->info.width
				<= file->info.width * file->info.height, E_ERROR);
	ret = da->op.append_row(file, point);

	if (da->op.on_data_change)
		da->op.on_data_change(&da->file.write.left, &da->file.write.right);

	return ret;
}

e_int32 da_write_row(data_adapter_t *da, e_uint32 row_idx, point_t* point, int file_right)
		{
	e_int32 ret;
	e_assert(da && da->state && da->op.write_row && point, E_ERROR_INVALID_HANDLER);
	file_ptr_t * file;

	if (!file_right) {
		e_assert(da->file_info.mode==E_WRITE || da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.left;
	}
	else {
		e_assert(da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.right;
	}

	file->modify = 1;
	e_assert(row_idx < file->info.height, E_ERROR);
	ret = da->op.write_row(file, row_idx, point);

	if (da->op.on_data_change)
		da->op.on_data_change(&da->file.write.left, &da->file.write.right);

	return ret;
}

e_int32 da_write_column(data_adapter_t *da, e_uint32 column_idx, point_t* point,
		int file_right)
		{
	e_int32 ret;
	e_assert(da && da->state && da->op.write_column && point, E_ERROR_INVALID_HANDLER);
	file_ptr_t * file;

	if (!file_right) {
		e_assert(da->file_info.mode==E_WRITE || da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.left;
	}
	else {
		e_assert(da->file_info.mode==E_DWRITE, E_ERROR);
		file = &da->file.write.right;
	}

	file->modify = 1;
	e_assert(column_idx < file->info.height, E_ERROR);
	ret = da->op.write_column(file, column_idx, point);

	if (da->op.on_data_change)
		da->op.on_data_change(&da->file.write.left, &da->file.write.right);

	return ret;
}

static e_int32 register_file_format(file_op_t* op, int i) {
	e_assert(file_adpt[i].op.open && file_adpt[i].op.close, E_ERROR);
	(*op) = file_adpt[i].op;
	return E_OK;
}

////////////////////////////////////////////////////////////////////////////////
//PCL
static e_int32 inter_pcl_open(file_ptr_t *file)
		{
	if (file->info.mode == E_READ) {
		pcl::PointCloud<pcl::PointXYZ> * cloud = new pcl::PointCloud<pcl::PointXYZ>();
		if (pcl::io::loadPCDFile<pcl::PointXYZ>("test_pcd.pcd", *cloud) == -1) //打开点云文件
				{
			PCL_ERROR("Couldn't read file test_pcd.pcd\n");
			return (-1);
		}
		file->handle = (size_t) cloud;
		file->info.width = cloud->width;
		file->info.height = cloud->height;
	}
	else
	{
		//打开缓存
		pcl::PointCloud<pcl::PointXYZ>* cloud = new pcl::PointCloud<pcl::PointXYZ>();
		// 创建点云缓存
		cloud->width = file->info.width;
		cloud->height = file->info.height;
		cloud->is_dense = false;
		cloud->points.resize(cloud->width * cloud->height);
		file->handle = (size_t) cloud;
	}

	return E_OK;
}
static e_int32 inter_pcl_combine(file_ptr_t *file1, file_ptr_t *file2)
		{
	pcl::PointCloud<pcl::PointXYZ> *cloud1 =
			(pcl::PointCloud<pcl::PointXYZ> *) (pcl::PointCloud<pcl::PointXYZ> *) file1->handle;
	pcl::PointCloud<pcl::PointXYZ> *cloud2 =
			(pcl::PointCloud<pcl::PointXYZ> *) (pcl::PointCloud<pcl::PointXYZ> *) file2->handle;

	(*cloud1) += (*cloud2);

	file2->info.mode = -1; //不需保存
	return E_OK;
}
static e_int32 inter_pcl_close(file_ptr_t *file) {
	pcl::PointCloud<pcl::PointXYZ> *cloud =
			(pcl::PointCloud<pcl::PointXYZ> *) file->handle;
	if (file->info.mode == E_WRITE) {
		pcl::io::savePCDFileASCII(file->info.file_name, *cloud);
	}
	delete cloud;
	return E_OK;
}
static e_int32 inter_pcl_write_point(file_ptr_t *file, int x, int y, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_pcl_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_pcl_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point) {
	return E_ERROR;
}

static point_xyz_t* transfor2xyz(point_t* pnts, int i, point_xyz_t *xyz) {
	point_polar_t *polar = (point_polar_t*) &pnts->mem;
	switch (pnts->type) {
	case PNT_TYPE_XYZ:
		return ((point_xyz_t*) &pnts->mem) + i;
	case PNT_TYPE_POLAR:
		hd_polar2xyz(&xyz->x, &xyz->y, &xyz->z,
						polar[i].distance, polar[i].angle_h, polar[i].angle_v);
		return xyz;
	default:
		DMSG((STDOUT,"transfor2xyz ERROR \n"));
		return NULL;
	}
}

static e_int32 inter_pcl_append_points(file_ptr_t *file, point_t* pnts, int pt_num) {
	pcl::PointCloud<pcl::PointXYZ> *cloud =
			(pcl::PointCloud<pcl::PointXYZ> *) (pcl::PointCloud<pcl::PointXYZ> *) file->handle;
	int idx = 0;
	point_xyz_t xyz, *pxyz;

	while (pt_num--)
	{
		pxyz = transfor2xyz(pnts, idx, &xyz);
		cloud->points[file->cousor].x = pxyz->x;
		cloud->points[file->cousor].y = pxyz->y;
		cloud->points[file->cousor].z = pxyz->z;
		file->cousor++;
		idx++;
	}
	return E_OK;

}
static e_int32 inter_pcl_append_row(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_pcl_append_column(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}

static e_int32 inter_pcl_read_points(file_ptr_t *file, point_t* pnts, int buf_len) {
	pcl::PointCloud<pcl::PointXYZ> *cloud =
			(pcl::PointCloud<pcl::PointXYZ> *) file->handle;
	point_xyz_t *point = (point_xyz_t*) &pnts->mem;
	while (buf_len--)
	{
		point->x = cloud->points[file->cousor].x;
		point->y = cloud->points[file->cousor].y;
		point->z = cloud->points[file->cousor].z;
		point++;
		file->cousor++;
	}
	return E_OK;
}
static e_int32 inter_pcl_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf) {
	return E_ERROR;
}
static e_int32 inter_pcl_read_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* buf) {
	return E_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
//JPG
static e_int32 inter_jpg_open(file_ptr_t *file)
		{
	if (file->info.mode == E_READ) {
//		pcl::PointCloud < pcl::PointXYZ > *cloud = new pcl::PointCloud<pcl::PointXYZ>();
//		if (pcl::io::loadPCDFile < pcl::PointXYZ > ("test_pcd.pcd", *cloud) == -1) //打开点云文件
//				{
//			PCL_ERROR("Couldn't read file test_pcd.pcd\n");
//			return (-1);
//		}
//		file->handle = (size_t) cloud;
//		file->info.width = cloud->width;
//		file->info.height = cloud->height;
		return E_ERROR;
	}
	else
	{
		//打开缓存
		e_uint8 *jpg = (e_uint8 *) malloc(file->info.width * file->info.height);
		file->handle = (size_t) jpg;
	}

	return E_OK;
}
static e_int32 inter_jpg_combine(file_ptr_t *file1, file_ptr_t *file2)
		{
	int width, height;
	e_uint8 *jpg1 = (e_uint8 *) file1->handle;
	e_uint8 *jpg2 = (e_uint8 *) file2->handle;

//TODO:支持多种合并,等高合并,按行合并,按列合并
	e_assert(file1->info.width == file2->info.width, E_ERROR_INVALID_PARAMETER);

//TODO:这里可以采用虚拟合并的方式,只保存索引,可减少内存使用.
	width = file1->info.width;
	height = file1->info.height + file2->info.height;

	e_uint8 *jpg = (e_uint8 *) malloc(width * height);
	memcpy(jpg, jpg1, file1->info.width * file1->info.height);
	memcpy(jpg + file1->info.width * file1->info.height, jpg2,
			file2->info.width * file2->info.height);

//调整合并后的高宽
	file1->info.width = width;
	file1->info.height = height;
	free(jpg1);
	file1->handle = (size_t) jpg;

	file2->info.mode = -1; //不需保存
	return E_OK;
}

int _save2image(char *filename, unsigned char *bits, int width, int height,
		int depth)
		{
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

	cinfo.image_width = width; // image width and height, in pixels
	cinfo.image_height = height;
	cinfo.input_components = 1; // # of color components per pixel
	cinfo.in_color_space = JCS_GRAYSCALE; //colorspace of input image
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE); // limit to baseline-JPEG values
	jpeg_start_compress(&cinfo, TRUE);
	row_stride = width * depth; // JSAMPLEs per row in image_buffer
	while (cinfo
			.next_scanline < cinfo.image_height) {
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

static int _save2image_rotation(char *filename, unsigned char *bits, int width,
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

static e_int32 inter_jpg_close(file_ptr_t *file) {
	e_uint8 *jpg = (e_uint8 *) file->handle;
	if (file->info.mode == E_WRITE) {
		_save2image_rotation(file->info.file_name, jpg, file->info.width, file->info.height);
	}
	free(jpg);
	return E_OK;
}
static e_int32 inter_jpg_write_point(file_ptr_t *file, int x, int y, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_jpg_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_jpg_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point) {
	return E_ERROR;
}
static e_int32 inter_jpg_append_points(file_ptr_t *file, point_t* pnts, int pt_num) {
	e_uint8 *jpg = (e_uint8 *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;
	while (pt_num--)
	{
		jpg[file->cousor] = point->gray;
		point++;
		file->cousor++;
	}
	return E_OK;
}
static e_int32 inter_jpg_append_row(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_jpg_append_column(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}

static e_int32 inter_jpg_read_points(file_ptr_t *file, point_t* pnts, int buf_len) {
	e_uint8 *jpg = (e_uint8 *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;
	while (buf_len--)
	{
		point->gray = jpg[file->cousor];
		point++;
		file->cousor++;
	}
	return E_OK;
}
static e_int32 inter_jpg_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf) {
	return E_ERROR;
}
static e_int32 inter_jpg_read_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* buf) {
	return E_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
//JPG
typedef struct jpg_private_t {
	gif_t gif;
	e_uint8 *buf;
} jpg_private_t;
static e_int32 inter_gif_open(file_ptr_t *file)
		{
	e_int32 ret;
	if (file->info.mode == E_READ) {
//		pcl::PointCloud < pcl::PointXYZ > *cloud = new pcl::PointCloud<pcl::PointXYZ>();
//		if (pcl::io::loadPCDFile < pcl::PointXYZ > ("test_pcd.pcd", *cloud) == -1) //打开点云文件
//				{
//			PCL_ERROR("Couldn't read file test_pcd.pcd\n");
//			return (-1);
//		}
//		file->handle = (size_t) cloud;
//		file->info.width = cloud->width;
//		file->info.height = cloud->height;
		return E_ERROR;
	}
	else
	{
		//打开文件
		jpg_private_t *gif = (jpg_private_t *) malloc(sizeof(jpg_private_t));
		gif->buf = (e_uint8*) calloc(file->info.width, file->info.height);
		ret =
				gif_open(&gif->gif, file->info.file_name, file->info.width, file->info.height, E_WRITE);
		e_assert(ret > 0, ret);

		ret = gif_put_header(&gif->gif);
		if (e_failed(ret)) {
			gif_close(&gif->gif);
			return ret;
		}

		file->handle = (size_t) gif;
	}

	return E_OK;
}

static e_int32 inter_gif_combine(file_ptr_t *file1, file_ptr_t *file2)
		{
	int ret;
	jpg_private_t *gif1 = (jpg_private_t *) file1->handle;
	jpg_private_t *gif2 = (jpg_private_t *) file2->handle;

//TODO:支持多种合并,等高合并,按行合并,按列合并
	e_assert(file1->info.width == file2->info.width
				&& file1->info.height == file2->info.height, E_ERROR_INVALID_PARAMETER);

#if 0
	//合并
	gif_append(&gif1->gif, &gif2->gif);
	//干掉gif2
	fi_delete((char*) gif2->gif.file_name);

#else
	//打开文件
	jpg_private_t *gif = (jpg_private_t *) malloc(sizeof(jpg_private_t));
	gif->buf =
			(e_uint8*) calloc(file1->info.width + file2->info.width, file1->info.height);

	ret = gif_union(&gif->gif, &gif1->gif, &gif2->gif);
	e_assert(ret > 0, ret);
	gif_close(&gif1->gif);
	fi_delete((char*) gif1->gif.file_name);
	fi_delete((char*) gif2->gif.file_name);
	free(gif1->buf);
	free(gif1);
	file1->handle = (size_t) gif;
#endif

	file2->info.mode = -1; //不需保存
	return E_OK;
}

static e_int32 inter_gif_close(file_ptr_t *file) {
	int ret = E_OK;
	jpg_private_t *gif = (jpg_private_t *) file->handle;
	gif_close(&gif->gif);

	if (strcmp((char*) gif->gif.file_name, (char*) file->info.file_name)) {
		fi_delete((char*) file->info.file_name);
		ret = fi_rename((char*) file->info.file_name, (char*) gif->gif.file_name);
		if (!ret)
			ret = E_ERROR;
		else
			ret = E_OK;
	}

	free(gif->buf);
	free(gif);
	return ret;
}
static e_int32 inter_gif_write_point(file_ptr_t *file, int x, int y, point_t* point) {
	return E_ERROR;
}

static e_int32 _insert_frame(file_ptr_t *file) {
	int ret;
	e_uint32 j;
	e_uint8 *pbuf;
	jpg_private_t *gif = (jpg_private_t *) file->handle;

	ret = gif_put_image(&gif->gif); //产生帧
	e_assert(ret > 0, ret);

	for (pbuf = gif->buf, j = 0;
			j < file->info.height;
			j++, pbuf += file->info.width)
			{
		gif_put_scan_line(&gif->gif, pbuf);
		e_assert(ret > 0, ret);
	}
	return E_OK;
}

static e_int32 inter_gif_write_row(file_ptr_t *file, e_uint32 row_idx, point_t* pnts) {
	int ret;
	jpg_private_t *gif = (jpg_private_t *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;

	//add one row
	for (unsigned int i = 0; i < file->info.width; i++)
			{
		gif->buf[row_idx * file->info.width + i] = point->gray;
		point++;
		file->cousor++;
	}

	ret = _insert_frame(file);
	e_assert(ret > 0, ret);

	return E_OK;
}
static e_int32 inter_gif_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* point) {
	return E_ERROR;
}
static e_int32 inter_gif_append_points(file_ptr_t *file, point_t* point, int pt_num) {
	e_uint8 *gif = (e_uint8 *) file->handle;
	return E_ERROR;
}
static e_int32 inter_gif_append_row(file_ptr_t *file, point_t* pnts) {
	int ret;
	jpg_private_t *gif = (jpg_private_t *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;
	//add one row
	for (unsigned int i = 0; i < file->info.width; i++)
			{
		gif->buf[file->cousor++] = point->gray;
		point++;

	}
	ret = _insert_frame(file);
	e_assert(ret > 0, ret);

	return E_OK;
}
static e_int32 inter_gif_append_column(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}

static e_int32 inter_gif_read_points(file_ptr_t *file, point_t* point,
		int buf_len) {
//	e_uint8 *gif = (e_uint8 *) file->handle;
	return E_ERROR;
}
static e_int32 inter_gif_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf) {
	return E_ERROR;
}
static e_int32 inter_gif_read_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* buf) {
	return E_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
//SPRITE
// server open -> write -> sendframe
// client readframe

typedef struct sprite_private_t {
	hd_connect_t connect;
	e_uint8 *buf;
	e_uint32 size;
} sprite_private_t;
static e_int32 inter_sprite_open(file_ptr_t *file)
		{
	int ret, size, h, w;
	e_uint8 buf[64];
	//打开缓存
	sprite_private_t *sprite = (sprite_private_t *) calloc(sizeof(sprite_private_t), 1);
	size = file->info.width * file->info.height;
	if (file->is_main) {
		ret = sc_open_pipe(&sprite->connect, file->info.file_name, size);
		if (e_failed(ret)) {
			free(sprite);
			return ret;
		}
		w = file->info.mode == E_DWRITE ? file->info.width * 2 : file->info.width;
		h = file->info.height;
		//发送长宽信息
		sprintf((char*) buf, "ABCD%05d%05dEFGH", w, h);
		ret = sc_send(&sprite->connect, buf, 18);
		if (e_failed(ret)) {
			free(sprite);
			return ret;
		}
	}

	if (file->is_main && file->info.mode == E_DWRITE) //主文件用于文件合并
			{
		sprite->buf = (e_uint8 *) calloc(size, 2);
	}
	else
	{
		sprite->buf = (e_uint8 *) calloc(size, 1);
	}

	sprite->size = size;
	file->handle = (size_t) sprite;
	return E_OK;
}

static void printfv(char* buf, int len) {
	char c, l = '0';
	int count = 0;

	while (len--) {
		c = *buf++;
		if (c == l) {
			count++;
		} else {
			if (count > 0)
				printf(" %c[%d]", l == 0 ? '0' : l, count);
			l = c;
			count = 1;
		}
	}
	if (count > 0)
		printf(" %c[%d]", l == 0 ? '0' : l, count);
}

static void inter_sprite_text(e_uint8 *buf, int w, int h) {
	for (int i = 0; i < h; i++) {
		printfv((char*) buf, w);
		printf("\n");
		buf += w;
	}
}

static e_int32 inter_sprite_combine(file_ptr_t *file1, file_ptr_t *file2)
		{
	file_ptr_t *main_file = NULL;
	e_uint8 *src, *dst;
	unsigned int width, height;
	sprite_private_t *main_sprite, *sub_sprite;

	if (file1->is_main) {
		main_file = file1;
	}
	else if (file2->is_main) {
		main_file = file2;
		file2 = file1;
	}

	if (main_file->info.mode != E_DWRITE)
		return E_OK;
	e_assert(file1->info.width == file2->info.width &&
				file1->info.height == file1->info.height, E_ERROR);

	if (!file2->modify)
		return E_OK;

	main_sprite = (sprite_private_t *) main_file->handle;
	sub_sprite = (sprite_private_t *) file2->handle;
	width = main_file->info.width;
	height = main_file->info.height;

//	DMSG((STDOUT,"\nmain_sprite:\n"));
//	inter_sprite_text(main_sprite->buf, width * 2, height);
//	DMSG((STDOUT,"\nsub_sprite:\n"));
//	inter_sprite_text(sub_sprite->buf, width, height);

	e_assert(main_file, E_ERROR);
	dst = main_sprite->buf + main_file->info.width;
	src = sub_sprite->buf;
	for (unsigned int i = 0; i < height; i++) {
		memcpy(dst, src, width);
		dst += width * 2;
		src += width;
	}

//	DMSG((STDOUT,"\nresult_sprite:\n"));
//	inter_sprite_text(main_sprite->buf, width * 2, height);

	file2->modify = 0;
	return E_OK;
}

static e_int32 inter_sprite_close(file_ptr_t *file) {
	sprite_private_t *sprite = (sprite_private_t *) file->handle;
	if (file->is_main)
		sc_close(&sprite->connect);
	free(sprite->buf);
	free(sprite);
	return E_OK;
}

static e_int32 _sprite_insert_frame(file_ptr_t *file) {
	int ret;
	sprite_private_t *sprite = (sprite_private_t *) file->handle;
	e_assert(file->is_main, E_ERROR);
	ret = sc_select(&sprite->connect, E_WRITE, 1E6);
	e_assert(ret > 0, ret);
	if (file->info.mode != E_DWRITE)
		ret = sc_send(&sprite->connect, sprite->buf, sprite->size);
	else
		ret = sc_send(&sprite->connect, sprite->buf, sprite->size * 2);
	e_assert(ret > 0, ret);
	return E_OK;
}
static e_int32 inter_sprite_write_point(file_ptr_t *file, int x, int y, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_sprite_write_row(file_ptr_t *file, e_uint32 row_idx,
		point_t* point) {
	return E_ERROR;
}
static e_int32 inter_sprite_write_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* pnts) {
	e_uint8 *pbuf;
	sprite_private_t *sprite = (sprite_private_t *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;
	e_assert(file->info.width > column_idx, E_ERROR);
	pbuf = &sprite->buf[column_idx];
	for (unsigned int i = 0; i < file->info.height; i++) {
		(*pbuf) = point->gray;
		pbuf += file->info.width;
		if (file->is_main)
			pbuf += file->info.width;
		point++;
	}
	return E_OK;
}
static e_int32 inter_sprite_append_points(file_ptr_t *file, point_t* pnts, int pt_num) {
	sprite_private_t *sprite = (sprite_private_t *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;
	while (pt_num--)
	{
		sprite->buf[file->cousor] = point->gray;
		point++;
		file->cousor++;
	}
	return E_OK;
}
static e_int32 inter_sprite_append_row(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}
static e_int32 inter_sprite_append_column(file_ptr_t *file, point_t* point) {
	return E_ERROR;
}

static e_int32 inter_sprite_read_points(file_ptr_t *file, point_t* pnts, int buf_len) {
	e_uint8 *sprite = (e_uint8 *) file->handle;
	point_gray_t *point = (point_gray_t*) &pnts->mem;
	while (buf_len--)
	{
		point->gray = sprite[file->cousor];
		point++;
		file->cousor++;
	}
	return E_OK;
}
static e_int32 inter_sprite_read_row(file_ptr_t *file, e_uint32 row_idx, point_t* buf) {
	return E_ERROR;
}
static e_int32 inter_sprite_read_column(file_ptr_t *file, e_uint32 column_idx,
		point_t* buf) {
	return E_ERROR;
}
static e_int32 inter_sprite_on_data_change(file_ptr_t *file1, file_ptr_t *file2)
		{
	e_int32 ret;
	ret = inter_sprite_combine(file1, file2);
	e_assert(ret>0, ret);

	if (file1->is_main) {
		ret = _sprite_insert_frame(file1);
	} else {
		ret = _sprite_insert_frame(file2);
	}
	e_assert(ret>0, ret);
	return E_OK;
}
