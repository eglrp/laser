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

#include "HLSDefinition.h"

#include <stdio.h>

#include <istream>
#include <fstream>
using namespace std;


class ByteStreamOut;

namespace hd
{
class LAS_API HLSWriter
{
public:
	HLSheader m_header;			//文件头信息
	//! 当前已经写入点数,close时会更新到m_header.number_of_point_records
	U64 m_pcount;
	//! 当前已经写入列数，close时会更新到m_header.number_of_col/number_of_row
	U32 m_pcol;
public:
	HLSWriter(void);
	~HLSWriter(void);
	//! 打开点云文件进行写入,header中的成员除了点数、行列数、范围外的参数都需要定义
	BOOL write(const char* file_name, const HLSheader* header,U32 io_buffer_size=65536);
	//! 写入一个扫描列,ptBuffer坐标点数组,count点个数,type类型0代表0~180写入主文件,1代表180~360写入副文件
	BOOL write_point(const HLSPointStru* ptBuffer,U32 count,U8 type);
	//! 关闭,并手动传入点数、行、列数。
	I64 close(U64 count,U32 row,U32 col);

private:
	ByteStreamOut* m_stream;
	char  m_file1path[1024];	//副文件路径
	FILE* m_file;				//主文件
	FILE* m_file1;				//副文件	
};
}