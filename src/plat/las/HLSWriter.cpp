#include <las/HLSWriter.h>

#include <laslib/src/bytestreamout_nil.hpp>
#include <laslib/src/bytestreamout_file.hpp>
#include <laslib/src/bytestreamout_ostream.hpp>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <Windows.h>
#endif

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
//#include <direct.h>
//#include <io.h>

const int IO_BUFFER_SIZE = 65536;

namespace hd
{

HLSWriter::HLSWriter(void)
{
	m_stream = 0;
	m_file = 0;
	m_file1 = 0;
	
	m_pcount = 0;
	m_pcol = 0;

	strcpy(m_file1path,"");
}


HLSWriter::~HLSWriter(void)
{
	if (m_stream) close();
}

BOOL HLSWriter::create( const char* file_name, const HLSheader* header,BOOL bUpdateHeader, U32 io_buffer_size/*=65536*/ )
{
	if (file_name == 0)
	{
		fprintf(stderr,"ERROR: file name pointer is zero\n");
		return FALSE;
	}

	m_header = (*header);
	m_header.min_x = F32_MAX;
	m_header.min_y = F32_MAX;
	m_header.min_z = F32_MAX;

	m_header.max_x = F32_MIN;
	m_header.max_y = F32_MIN;
	m_header.max_z = F32_MIN;

	// 获得时间
//	SYSTEMTIME tm;
//	GetSystemTime(&tm);
//	m_header.file_creation_day = GetDayOfYear(tm.wMonth,tm.wDay);
//	m_header.file_creation_year = tm.wYear;

	strcpy(m_file1path, file_name);
	strcat(m_file1path,"1");

	//若文件夹目录不存在则新建相应文件夹
	char* tag;
	char* fileTst = new char[1000];
	strcpy(fileTst,file_name);
    string filenameStr(fileTst);
	string tmp = filenameStr.substr(0,filenameStr.find_last_of('\\') + 1);
	
	const char* fileTst1 = tmp.c_str();
	strcpy(fileTst,fileTst1);

	for (tag = fileTst;*tag;tag++)
	{
		char* escStr = "\\";
		if (strcmp(tag,escStr) == 0)
		{
			char buffer[1000],path[1000];
			strcpy(buffer,fileTst);
			buffer[strlen(fileTst) - strlen(tag) + 1] = NULL;
			strcpy(path,buffer);
			if (access(path,6) == -1)
			{
				mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			}
		}
	}

	m_file = fopen(file_name, bUpdateHeader ? "r+b" : "w+b");//"w+b"
	if (m_file == 0)
	{
		fprintf(stderr, "ERROR: cannot open file '%s'\n", file_name);
		return FALSE;
	}	
	fseek(m_file,0,SEEK_SET);

	if (setvbuf(m_file, NULL, _IOFBF, io_buffer_size) != 0)
	{
		fprintf(stderr, "WARNING: setvbuf() failed with buffer size %u\n", io_buffer_size);
	}

	ByteStreamOut* out;
	if (IS_LITTLE_ENDIAN())
		out = new ByteStreamOutFileLE(m_file);
	else
		out = new ByteStreamOutFileBE(m_file);

	BOOL bRet = write(out, &m_header);
	if (bUpdateHeader)
	{
		close(m_header.number_of_point_records,m_header.number_of_row,m_header.number_of_col);
	}

	delete []fileTst;
	return bRet;
}

BOOL HLSWriter::write( FILE* file, const HLSheader* header)
{
	if (file == 0)
	{
		fprintf(stderr,"ERROR: file pointer is zero\n");
		return FALSE;
	}

#ifdef _WIN32
	if (file == stdout)
	{
		if(_setmode( _fileno( stdout ), _O_BINARY ) == -1 )
		{
			fprintf(stderr, "ERROR: cannot set stdout to binary (untranslated) mode\n");
		}
	}
#endif

	ByteStreamOut* out;
	if (IS_LITTLE_ENDIAN())
		out = new ByteStreamOutFileLE(file);
	else
		out = new ByteStreamOutFileBE(file);

	return write(out, header);
}

BOOL HLSWriter::write( ByteStreamOut* stream, const HLSheader* header)
{
	U32 i;

	if (stream == 0)
	{
		fprintf(stderr,"ERROR: ByteStreamOut pointer is zero\n");
		return FALSE;
	}
	this->m_stream = stream;
	
	if (header == 0)
	{
		fprintf(stderr,"ERROR: LASheader pointer is zero\n");
		return FALSE;
	}
	// 检查文件头信息
	if (!header->check()) return FALSE;

	//m_point_data_format = header->point_data_format;
	// 开始写入文件头
	char hdBuf[256] = {0};
	header->serialize(hdBuf);
	if (!stream->putBytes((U8*)hdBuf, 256))
	{
		fprintf(stderr,"ERROR: writing header\n");
		return FALSE;
	}
	fseek(m_file,256,SEEK_SET);
	return TRUE;
}
//! 写入地面站扫描仪点
BOOL HLSWriter::write_point( const HLSpoint* point )
{
	if (!((m_header.point_data_format >= HLS_POINTFORMAT_XYZI && 
		m_header.point_data_format <= HLS_POINTFORMAT_XYZIRGBP) ||
		(m_header.point_data_format >= HLS2_POINTFORMAT_XYZI && 
		m_header.point_data_format <= HLS2_POINTFORMAT_XYZIRGBP)))
	{
		return FALSE;
	}
	try
	{
		m_stream->put32bitsLE((U8*)&(point->x));
		m_stream->put32bitsLE((U8*)&(point->y));
		m_stream->put32bitsLE((U8*)&(point->z));
		m_stream->put16bitsLE((U8*)&(point->intensity));
		if (m_header.point_data_format >= HLS_POINTFORMAT_XYZI && 
			m_header.point_data_format <= HLS_POINTFORMAT_XYZIRGBP)
		{
			m_stream->put16bitsLE((U8*)&(point->point_source_ID));
		}
		if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			m_stream->putByte((point->rgb[0]));
			m_stream->putByte((point->rgb[1]));
			m_stream->putByte((point->rgb[2]));
		}
		if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			m_stream->putByte((point->classification));
			//m_stream->put64bitsLE((U8*)&(point->gps_time));
		}
		m_pcount++;
		// 更新范围
		if (m_header.point_data_format != 1)
		{
			updateExtent(point->x,point->y,point->z);
		}
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}	
}

//! 写入车载格式点
BOOL HLSWriter::write_point2( F64 x,F64 y,F64 z,U16 intensity )
{
	if (!((m_header.point_data_format >= HLS2_POINTFORMAT_XYZI && 
		m_header.point_data_format <= HLS2_POINTFORMAT_XYZIRGBP)))
	{
		return FALSE;
	}

	try
	{
		F32 x32 = x - m_header.offsetX;
		F32 y32 = y - m_header.offsetY;
		F32 z32 = z - m_header.offsetZ;

		m_stream->put32bitsLE((U8*)&(x32));
		m_stream->put32bitsLE((U8*)&(y32));
		m_stream->put32bitsLE((U8*)&(z32));
		m_stream->put16bitsLE((U8*)&(intensity));

		if (m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			U8 tmp = 0;
			m_stream->put16bitsLE((U8*)&(tmp));
			m_stream->put16bitsLE((U8*)&(tmp));
			m_stream->put16bitsLE((U8*)&(tmp));
		}

		if (m_header.point_data_format >= HLS2_POINTFORMAT_XYZIRGBP)
		{
			m_stream->putByte(0);	//写入分类属性
			F64 gpsTime = 0;
			m_stream->put64bitsLE((U8*)&(gpsTime));
		}

		m_pcount++;
		// 更新范围
		updateExtent(x32,y32,z32);
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
}

//! 写入车载格式点
BOOL HLSWriter::write_point2( F64 x,F64 y,F64 z,U16 intensity,U16 range,U16 angle)
{
	if (!((m_header.point_data_format >= HLS2_POINTFORMAT_XYZI && 
		m_header.point_data_format <= HLS2_POINTFORMAT_XYZIRA)))
	{
		return FALSE;
	}

	try
	{
		F32 x32 = x - m_header.offsetX;
		F32 y32 = y - m_header.offsetY;
		F32 z32 = z - m_header.offsetZ;

		m_stream->put32bitsLE((U8*)&(x32));
		m_stream->put32bitsLE((U8*)&(y32));
		m_stream->put32bitsLE((U8*)&(z32));
		m_stream->put16bitsLE((U8*)&(intensity));
		m_stream->put16bitsLE((U8*)&(range));
		m_stream->put16bitsLE((U8*)&(angle));

		if (m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			U8 tmp = 0;
			m_stream->put16bitsLE((U8*)&(tmp));
			m_stream->put16bitsLE((U8*)&(tmp));
			m_stream->put16bitsLE((U8*)&(tmp));
		}

		if (m_header.point_data_format >= HLS2_POINTFORMAT_XYZIRA)
		{
			m_stream->putByte(0);	//写入分类属性
			F64 gpsTime = 0;
			m_stream->put64bitsLE((U8*)&(gpsTime));
		}

		m_pcount++;
		// 更新范围
		updateExtent(x32,y32,z32);
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
}

//! 写入地面站点
BOOL HLSWriter::write_point( F32 x,F32 y,F32 z,U16 intensity,U16 sourceID /*= 0*/ )
{
	if (!((m_header.point_data_format >= HLS_POINTFORMAT_XYZI && 
		m_header.point_data_format <= HLS_POINTFORMAT_XYZIRGBP) ||
		(m_header.point_data_format >= HLS2_POINTFORMAT_XYZI && 
		m_header.point_data_format <= HLS2_POINTFORMAT_XYZIRGBP)))
	{
		return FALSE;
	}
	try
	{
		m_stream->put32bitsLE((U8*)&(x));
		m_stream->put32bitsLE((U8*)&(y));
		m_stream->put32bitsLE((U8*)&(z));
		m_stream->put16bitsLE((U8*)&(intensity));
		// 只有地面站才写入点源ID
		if (m_header.point_data_format >= HLS_POINTFORMAT_XYZI &&
			m_header.point_data_format <= HLS_POINTFORMAT_XYZIRGBP)
		{
			m_stream->put16bitsLE((U8*)&(sourceID));
		}
		if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			U16 tmp = 0;
			m_stream->put16bitsLE((U8*)&(tmp));
			m_stream->put16bitsLE((U8*)&(tmp));
			m_stream->put16bitsLE((U8*)&(tmp));
		}
		if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			m_stream->putByte(0);
			F64 gpsTime = 0;
			m_stream->put64bitsLE((U8*)&(gpsTime));
		}
		m_pcount++;
		// 更新范围
		if (m_header.point_data_format != HLS_POINTFORMAT_RHVI)
		{
			updateExtent(x,y,z);
		}
		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}	
}
//! 写入地面站扫描仪点
BOOL HLSWriter::write_point( const HLSPointStru* ptBuffer,U32 count,U8 type)
{
	if (m_header.point_data_format != HLS_POINTFORMAT_XYZI && 
		m_header.point_data_format != HLS_POINTFORMAT_RHVI)
	{
		return FALSE;
	}
	try
	{
		// 检查文件头标记是否为格式0(直角坐标)或1(极坐标)
		/*if(m_header.point_data_format != 0 && 
			m_header.point_data_format != 1)
			return FALSE;*/

		if (type == 0)
		{
			fwrite((void*)ptBuffer,sizeof(HLSPointStru),count,m_file);
			m_pcol++;
			m_pcount += count;
			// 更新范围
			if (m_header.point_data_format != HLS_POINTFORMAT_RHVI)
			{
				for (U32 i = 0;i < count;i++)
				{
					updateExtent(ptBuffer[i].x,ptBuffer[i].y,ptBuffer[i].z);
				}
			}
			return TRUE;
		}
		else if(type == 1)
		{
			if(m_file1 == NULL)
			{
				m_file1 = fopen(m_file1path, "w+b");
				if (m_file1 == 0)
				{
					fprintf(stderr, "ERROR: cannot open file '%s'\n", m_file1path);
					return FALSE;
				}
				if (setvbuf(m_file1, NULL, _IOFBF, IO_BUFFER_SIZE) != 0)
				{
					fprintf(stderr, "WARNING: setvbuf() failed with buffer size %u\n", IO_BUFFER_SIZE);
				}
			}
			fwrite((void*)ptBuffer,sizeof(HLSPointStru),count,m_file1);
			m_pcount += count;
			m_pcol++;
			// 更新范围
			if (m_header.point_data_format == 0)
			{
				for (U32 i = 0;i < count;i++)
				{
					updateExtent(ptBuffer[i].x,ptBuffer[i].y,ptBuffer[i].z);
				}
			}
			return TRUE;
		}
	}
	catch (...)
	{
		return FALSE;
	}

	return FALSE;
}

BOOL HLSWriter::write_point( const HLSPointStru3* ptBuffer,U32 count,U8 type)
{
	if (m_header.point_data_format != HLS_POINTFORMAT_XYZIRGB)
	{
		return FALSE;
	}
	try
	{
		// 检查文件头标记是否为格式0(直角坐标)或1(极坐标)
		/*if(m_header.point_data_format != 0 && 
			m_header.point_data_format != 1)
			return FALSE;*/

		if (type == 0)
		{
			fwrite((void*)ptBuffer,sizeof(HLSPointStru3),count,m_file);
			m_pcol++;
			m_pcount += count;
			// 更新范围
			if (m_header.point_data_format != HLS_POINTFORMAT_RHVI)
			{
				for (U32 i = 0;i < count;i++)
				{
					updateExtent(ptBuffer[i].x,ptBuffer[i].y,ptBuffer[i].z);
				}
			}
			return TRUE;
		}
		else if(type == 1)
		{
			if(m_file1 == NULL)
			{
				m_file1 = fopen(m_file1path, "w+b");
				if (m_file1 == 0)
				{
					fprintf(stderr, "ERROR: cannot open file '%s'\n", m_file1path);
					return FALSE;
				}
				if (setvbuf(m_file1, NULL, _IOFBF, IO_BUFFER_SIZE) != 0)
				{
					fprintf(stderr, "WARNING: setvbuf() failed with buffer size %u\n", IO_BUFFER_SIZE);
				}
			}
			fwrite((void*)ptBuffer,sizeof(HLSPointStru3),count,m_file1);
			m_pcount += count;
			m_pcol++;
			// 更新范围
			if (m_header.point_data_format == 0)
			{
				for (U32 i = 0;i < count;i++)
				{
					updateExtent(ptBuffer[i].x,ptBuffer[i].y,ptBuffer[i].z);
				}
			}
			return TRUE;
		}
	}
	catch (...)
	{
		return FALSE;
	}

	return FALSE;
}

BOOL HLSWriter::chunk()
{
	return (fflush(m_file) == 0) && (fflush(m_file1) == 0);
}

BOOL HLSWriter::update_header()
{
	if (m_stream == 0)
	{
		fprintf(stderr,"ERROR: stream pointer is zero\n");
		return FALSE;
	}
	if (!m_stream->isSeekable())
	{
		fprintf(stderr,"ERROR: stream is not seekable\n");
		return FALSE;
	}
	m_stream->seek(0);
	write(m_stream,&m_header);

	m_stream->seekEnd();
	
	return TRUE;
}

I64 HLSWriter::close()
{
	I32 row = m_pcol <= 0 ? m_pcol : m_pcount / m_pcol;
	return close(m_pcount,row,m_pcol);
}

I64 HLSWriter::close( U64 count,U32 row,U32 col )
{
	I64 bytes = 0;

	m_header.number_of_point_records = count;
	m_header.number_of_col = col;
	m_header.number_of_row = row;

	// 合并主副文件
	combine();
	// 获取合并后的主文件长度
	bytes = ftell(m_file);

	// 更新文件头
	update_header();
	
	if (m_stream)
	{
		delete m_stream;
		m_stream = 0;
	}

	// 关闭主文件
	if (m_file)
	{
		fclose(m_file);
		m_file = 0;
	}
	// 关闭副文件，并删除
	if (m_file1)
	{
		fclose(m_file1);
		m_file1 = 0;
		remove(m_file1path);
	}
	m_pcount = 0;

	return bytes;
}

BOOL HLSWriter::combine()
{
	if(m_file != NULL && m_file1 != NULL)
	{
		fflush(m_file1);
		fseek(m_file1,0,SEEK_END);
		long len = ftell(m_file1);
		fseek(m_file1,0,SEEK_SET);

		char buffer[IO_BUFFER_SIZE];
		long i;
		for (i = 0;i + IO_BUFFER_SIZE <= len; i += IO_BUFFER_SIZE)
		{
			fread(buffer,1,IO_BUFFER_SIZE,m_file1);
			fwrite(buffer,1,IO_BUFFER_SIZE,m_file);
		}

		if (i < len)
		{
			fread(buffer,1,len - i,m_file1);
			fwrite(buffer,1,len - i,m_file);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL HLSWriter::seek( const I64 p_index )
{
	if (m_stream == NULL)
	{
		return FALSE;
	}

	try { m_stream->seek(256 + p_index * m_header.point_data_record_length); } catch(...)
	{
		fprintf(stderr,"ERROR: seek to p_index\n");
		return FALSE;
	}
	return TRUE;
}

}
