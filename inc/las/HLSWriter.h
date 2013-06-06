/*! HLSWriter.h
 ********************************************************************************
 <PRE>
 模块名       : LASLib
 文件名       : HLSWriter.h
 相关文件     :
 文件实现功能 : 海达数云点文件hls写入
 作者         : 龚书林
 版本         : 1.0
 --------------------------------------------------------------------------------
 备注         : <其它说明>
 --------------------------------------------------------------------------------
 修改记录 :
 日 期        版本     修改人              修改内容
 2012/05/30   1.0      龚书林			  原始版本
 2012/06/16   1.01     龚书林			  增加自动更新文件头功能,
 不需要调用者记录行列数、点数、范围进行更新
 </PRE>
 *******************************************************************************/

#pragma once

#include "hlsDefinition.h"

#include <stdio.h>
//
//#include <istream>
//#include <fstream>
//#include <string>
using namespace std;

class ByteStreamOut;

namespace hd
{
class HLS_API HLSWriter
{
public:
	HLSheader m_header; //文件头信息
	//! 当前已经写入点数,close时会更新到m_header.number_of_point_records
	U64 m_pcount;
	//! 当前已经写入列数,close时会更新到m_header.number_of_col/number_of_row
	U32 m_pcol;
public:
	HLSWriter(void);
	~HLSWriter(void);
	//! 打开点云文件进行写入,header中的成员除了点数、行列数、范围外的参数都需要定义
	BOOL create(const char* file_name, const HLSheader* header,
			BOOL bUpdateHeader = FALSE, U32 io_buffer_size = 65536);
	//! 写入一个点记录,地面站格式
	BOOL write_point(const HLSpoint* point);
	//! 写入一个点记录,地面站格式
	BOOL write_point(F32 x, F32 y, F32 z, U16 intensity, U16 sourceID = 0);
	//! 写入一个地面站扫描列,ptBuffer坐标点数组,count点个数,type类型0代表0~180写入主文件,1代表180~360写入副文件
	BOOL write_point(const HLSPointStru* ptBuffer, U32 count, U8 type);

	BOOL write_point(const HLSPointStru3* ptBuffer, U32 count, U8 type);
	//! 写入一个点记录,车载格式
	BOOL write_point2(F64 x, F64 y, F64 z, U16 intensity);
	//! 写入一个点记录，车载格式，包括角度、距离和强度等原始信息
	BOOL write_point2(F64 x, F64 y, F64 z, U16 intensity, U16 range, U16 angle);
	//! 跳转到指定点,p_index为点序号
	BOOL seek(const I64 p_index);
	//! 强制保存,使得IO缓存的内容写入文件
	BOOL chunk();
	//! 关闭,自动更新文件头,同时合并主副文件
	I64 close();
	//! 关闭,并手动传入点数、行、列数。主要用于无序写入方式
	I64 close(U64 count, U32 row, U32 col);
private:
	//! 结束时合并主副文件
	BOOL combine();
	//! 更新文件头，用于更新记录个数、范围、扫描行列数
	BOOL update_header();
	//! 更新范围
	inline void updateExtent(F32 x, F32 y, F32 z)
	{
		m_header.max_x = MAX(x, m_header.max_x);
		m_header.min_x = MIN(x, m_header.min_x);
		m_header.max_y = MAX(y, m_header.max_y);
		m_header.min_y = MIN(y, m_header.min_y);
		m_header.max_z = MAX(z, m_header.max_z);
		m_header.min_z = MIN(z, m_header.min_z);
	}
private:
	BOOL write(FILE* file, const HLSheader* header);
	BOOL write(ByteStreamOut* stream, const HLSheader* header);
	ByteStreamOut* m_stream;
	char m_file1path[1024]; //副文件路径
	FILE* m_file; //主文件
	FILE* m_file1; //副文件
};

}
