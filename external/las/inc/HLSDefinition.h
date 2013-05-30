/*! hlsDefinition.h
 ********************************************************************************
 <PRE>
 模块名       : LASLib
 文件名       : HLSDefinition.h
 相关文件     :
 文件实现功能 : 海达数云点文件hls读写模块基本类型定义
 作者         : 龚书林
 版本         : 1.0
 --------------------------------------------------------------------------------
 备注         : <其它说明>
 --------------------------------------------------------------------------------
 修改记录 :
 日 期        版本     修改人              修改内容
 2012/05/30   1.0      龚书林
 </PRE>
 *******************************************************************************/

#ifndef HLS_DEFINITIONS_HPP
#define HLS_DEFINITIONS_HPP

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "mydefs.hpp"
//定义点记录格式
#define HLS_POINTFORMAT0  0
#define HLS_POINTFORMAT1  1
#define HLS_POINTFORMAT2  2
//定义属性字段类型
#define HLS_ATTRIBUTE_U8  0
#define HLS_ATTRIBUTE_I8  1
#define HLS_ATTRIBUTE_U16 2
#define HLS_ATTRIBUTE_I16 3
#define HLS_ATTRIBUTE_U32 4
#define HLS_ATTRIBUTE_I32 5
#define HLS_ATTRIBUTE_U64 6
#define HLS_ATTRIBUTE_I64 7
#define HLS_ATTRIBUTE_F32 8
#define HLS_ATTRIBUTE_F64 9
//定义HLS格式版本号
#define HLSFORMAT_VERSION_MAJOR 1
#define HLSFORMAT_VERSION_MINOR 0

namespace hd {
//! 坐标点结构体
struct HLSPointStru {
	HLSPointStru() {
		x = y = z = 0.0;
		intensity = 0;
		point_source_ID = 0;
	}
	float x; // 坐标X或距离
	float y; // 坐标Y或水平角
	float z; // 坐标Z或垂直角
	U16 intensity; // 反射强度
	U16 point_source_ID; // 点源ID
};

class LAS_API HLSpoint {
public:
	//记录格式0
	float x;
	float y;
	float z;
	U16 intensity;
	U16 point_source_ID;
	//记录格式1
	U16 rgb[3];
	//记录格式2
	U8 classification;
	F64 gps_time;

	BOOL have_gps_time;
	BOOL have_rgb;

// copy functions

	HLSpoint(HLSpoint & other) {
		*this = other;
	}

	HLSpoint & operator=(const HLSpoint & other) {
		x = other.x;
		y = other.y;
		z = other.z;
		intensity = other.intensity;
		classification = other.classification;
		point_source_ID = other.point_source_ID;

		if (other.have_gps_time) {
			gps_time = other.gps_time;
		}
		if (other.have_rgb) {
			rgb[0] = other.rgb[0];
			rgb[1] = other.rgb[1];
			rgb[2] = other.rgb[2];
		}
		return *this;
	}

	BOOL init() {
		clean();

		return TRUE;
	}

	BOOL is_zero() const {
		if (((U32*) &(this->x))[0] || ((U32*) &(this->x))[1]
				|| ((U32*) &(this->x))[2] || ((U32*) &(this->x))[3]
				|| ((U32*) &(this->x))[4]) {
			return FALSE;
		}
		if (have_gps_time) {
			if (this->gps_time) {
				return FALSE;
			}
		}
		if (have_rgb) {
			if (this->rgb[0] || this->rgb[1] || this->rgb[2]) {
				return FALSE;
			}

		}
		return TRUE;
	}

	void zero() {
		x = 0;
		y = 0;
		z = 0;
		intensity = 0;
		classification = 0;
		point_source_ID = 0;

		gps_time = 0.0;
		rgb[0] = rgb[1] = rgb[2] = 0;
	}

	void clean() {
		zero();

		have_gps_time = FALSE;
		have_rgb = FALSE;
	}
	;

	HLSpoint() {
		clean();
	}
	;

	~HLSpoint() {
		clean();
	}

};

class LAS_API HLSheader {
public:
	CHAR file_signature[4]; // 文件标识,固定为"HLSF"
	U32 project_ID_GUID_data_1; // 工程唯一ID 1
	U16 project_ID_GUID_data_2;
	U16 project_ID_GUID_data_3;
	U8 project_ID_GUID_data_4[8];
	U16 file_source_id; // 文件源ID

	U8 version_major; // 格式主版本号
	U8 version_minor; // 格式次版本号
	CHAR system_identifier[32]; // 系统标识符
	CHAR generating_software[32]; // 生成软件，如hdScan
	U16 file_creation_day; // 文件生成日期，一年中的第几天
	U16 file_creation_year; // 文件生成年份
	U16 header_size; // 文件头大小,目前是256byte
	U32 offset_to_point_data; // 点记录区偏移量

	U8 point_data_format; // 点格式编号0~3,0代表直角坐标及反射强度,1代表极坐标及反射强度
	U16 point_data_record_length; // 一个点记录所占byte数,16,22,31
	U64 number_of_point_records; // 点记录数
	U32 number_of_row; // 扫描行数,如果非规则行列点云,则number_of_row=0,number_of_col=0
	U32 number_of_col; // 扫描列数

	F32 max_x; // 范围
	F32 min_x;
	F32 max_y;
	F32 min_y;
	F32 max_z;
	F32 min_z;

	F32 centerX; //测站扫描仪位置
	F32 centerY;
	F32 centerZ;
	F32 yaw; //扫描仪航向
	F32 pitch; //扫描仪前后翻滚角
	F32 roll; //扫描仪左右翻滚角

	CHAR Reversed[90]; // 保留位

	HLSheader() {
		clean_las_header();
	}

	// set bounding box

	void set_bounding_box(F64 min_x, F64 min_y, F64 min_z, F64 max_x, F64 max_y,
			F64 max_z) {
		this->min_x = (F32) min_x;
		this->min_y = (F32) min_y;
		this->min_z = (F32) min_z;
		this->max_x = (F32) max_x;
		this->max_y = (F32) max_y;
		this->max_z = (F32) max_z;
	}

	// clean functions

	void clean_las_header() {
		memset((void*) this, 0, sizeof(HLSheader));
		file_signature[0] = 'H';
		file_signature[1] = 'L';
		file_signature[2] = 'S';
		file_signature[3] = 'F';
		version_major = 1;
		version_minor = 0;
		header_size = 256;
		offset_to_point_data = 256;
		point_data_record_length = 16;
		// 默认极坐标
		point_data_format = 1;
	}

	void clean() {
		clean_las_header();
	}

	BOOL check() const {
		if (strncmp(file_signature, "HLSF", 4) != 0) {
			fprintf(stderr, "ERROR: wrong file signature '%s'\n",
					file_signature);
			return FALSE;
		}
		if ((version_major != 1) || (version_minor > 0)) {
			fprintf(stderr,
					"WARNING: unknown version %d.%d (should be 1.0 or 1.1 or 1.2 or 1.3 or 1.4)\n",
					version_major, version_minor);
		}
		if (header_size < 256) {
			fprintf(stderr,
					"ERROR: header size is %d but should be at least 256\n",
					header_size);
			return FALSE;
		}
		if (offset_to_point_data < header_size) {
			fprintf(stderr,
					"ERROR: offset to point data %d is smaller than header size %d\n",
					offset_to_point_data, header_size);
			return FALSE;
		}
		if (max_x < min_x || max_y < min_y || max_z < min_z) {
			fprintf(stderr,
					"WARNING: invalid bounding box [ %g %g %g / %g %g %g ]\n",
					min_x, min_y, min_z, max_x, max_y, max_z);
		}
		return TRUE;
	}

	~HLSheader() {
		clean();
	}

};

}
#endif
