/*! hlsDefinition.h
********************************************************************************
<PRE>
模块名       : LASLib
文件名       : hlsDefinition.h
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

#include <las/mydefs.hpp>
//定义点记录格式,地面扫描仪点格式
#define HLS_POINTFORMAT_XYZI  0x0		// xyzi记录
#define HLS_POINTFORMAT_RHVI  0x1		// 距离角度记录
#define HLS_POINTFORMAT_XYZIRGB	  0x2   // 带颜色点			0000 0010
#define HLS_POINTFORMAT_XYZIRGBP  0x4	// 带颜色和分类及GPS时间点 0000 0100
//定义HLS2.0扫描点格式
#define HLS2_POINTFORMAT_RHVI	    9		// 距离角度记录2
#define HLS2_POINTFORMAT_XYZ		10		// 2.0xyz点云		1
#define HLS2_POINTFORMAT_XYZI		11		// 2.0xyzi点云		1
#define HLS2_POINTFORMAT_XYZIRGB	12		// 2.0xyzirgb点云
#define HLS2_POINTFORMAT_XYZIRGBP	13		// 2.0xyzirgbp点云
#define HLS2_POINTFORMAT_XYZIRA		14      // 2.0xyzira点云

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

namespace hd
{
#pragma  pack(push,1)
	//! 地面站坐标点结构体
	struct HLSPointStru
	{
		HLSPointStru()
		{
			x = y = z = 0.0f;
			intensity = 0;
			point_source_ID = 0;
		}
		float  x;		// 坐标X或距离
		float  y;		// 坐标Y或水平角
		float  z;		// 坐标Z或垂直角
		U16 intensity;      // 反射强度
		U16 point_source_ID;// 点源ID
	};

	//! 车载坐标点结构体,绝对坐标
	struct HLSPointStru2
	{
		HLSPointStru2()
		{
			x = y = z = 0.0;
			intensity = 0;
		}
		double  x;			// 坐标X
		double  y;			// 坐标Y
		double  z;			// 坐标Z
		U16 intensity;      // 反射强度
	};

	struct HLSPointStru3
	{
		HLSPointStru3()
		{
			x = y = z = 0.0f;
			intensity = 0;
			point_source_ID = 0;
			r = g = b =0;
		}
		float  x;		// 坐标X或距离
		float  y;		// 坐标Y或水平角
		float  z;		// 坐标Z或垂直角
		U16 intensity;      // 反射强度
		U16 point_source_ID;// 点源ID
		U16 r;
		U16 g;
		U16 b;
	};
	

class HLS_API HLSpoint
{
public:
  //记录格式0
  float x;
  float y;
  float z;
  U16 intensity;
  U16 point_source_ID;
  //记录格式1
  U8 rgb[3];
  //记录格式2
  U8 classification;
  F64 gps_time;
 
  BOOL have_gps_time;
  BOOL have_rgb;
 
// copy functions

  HLSpoint(HLSpoint & other)
  {
    *this = other;
  }

  HLSpoint & operator=(const HLSpoint & other)
  {
    x = other.x;
    y = other.y;
    z = other.z;
    intensity = other.intensity;
    classification = other.classification;
    point_source_ID = other.point_source_ID;

    if (other.have_gps_time)
    {
      gps_time = other.gps_time;
    }
    if (other.have_rgb)
    {
      rgb[0] = other.rgb[0];
      rgb[1] = other.rgb[1];
      rgb[2] = other.rgb[2];
    }
    return *this;
  };

  BOOL init()
  {
    clean();

    return TRUE;
  };

 
  BOOL is_zero() const
  {
    if (((U32*)&(this->x))[0] || ((U32*)&(this->x))[1] || ((U32*)&(this->x))[2] || ((U32*)&(this->x))[3] || ((U32*)&(this->x))[4])
    {
      return FALSE;
    }
    if (have_gps_time)
    {
      if (this->gps_time)
      {
        return FALSE;
      }
    }
    if (have_rgb)
    {
      if (this->rgb[0] || this->rgb[1] || this->rgb[2])
      {
        return FALSE;
      }
      
    }
    return TRUE;
  }

  void zero()
  {
    x=0;
    y=0;
    z=0;
    intensity=0;
    classification = 0;
    point_source_ID = 0;

    gps_time = 0.0;
    rgb[0] = rgb[1] = rgb[2] = 0;
  };

  void clean()
  {
    zero();
	
    have_gps_time = FALSE;
    have_rgb = FALSE;
  };

  HLSpoint()
  {
    clean();
  };
  
  ~HLSpoint()
  {
    clean();
  };
};

class HLS_API HLSheader
{
	friend class HLSWriter;
	friend class HLSreader;
	friend class CHL1Reader;
	friend class CHL2Reader;
	friend class CHLS2Reader;
private:
  U8  point_data_format;		// 点格式编号0~3,11~13,参见点格式宏定义
  U16 point_data_record_length;	// 一个点记录所占byte数,16,22,31
public:
  CHAR file_signature[4];		// 文件标识,固定为"HLSF","HLS2","MLSF"
  U32 project_ID_GUID_data_1;	// 工程唯一ID 1
  U16 project_ID_GUID_data_2;
  U16 project_ID_GUID_data_3;
  U8  project_ID_GUID_data_4[8];
  U16 file_source_id;			// 文件源ID

  U8 version_major;				// 格式主版本号
  U8 version_minor;				// 格式次版本号
  CHAR system_identifier[32];	// 系统标识符
  CHAR generating_software[32];	// 生成软件，如hdScan
  U16 file_creation_day;		// 文件生成日期，一年中的第几天
  U16 file_creation_year;		// 文件生成年份
  U16 header_size;				// 文件头大小,目前是256byte
  U32 offset_to_point_data;		// 点记录区偏移量
  U64 number_of_point_records;	// 点记录数
  U32 number_of_row;			// 扫描行数,如果非规则行列点云,则number_of_row=0,number_of_col=0
  U32 number_of_col;			// 扫描列数

  F32 max_x;					// 偏移后的相对坐标范围
  F32 min_x;
  F32 max_y;
  F32 min_y;
  F32 max_z;
  F32 min_z;

  F32 centerX;			//测站扫描仪位置,单站扫描仪位置
  F32 centerY;			
  F32 centerZ;
  F32 yaw;				//扫描仪航向		,单站扫描仪
  F32 pitch;			//扫描仪前后翻滚角	,单站扫描仪
  F32 roll;				//扫描仪左右翻滚角	,单站扫描仪

  //! 转换坐标7参数
  F64 offsetX;			// 偏移量X,最终坐标是 X = offsetX + x;
  F64 offsetY;			// 偏移量Y
  F64 offsetZ;			// 偏移量Z
  F64 rotateX;			// 旋转X
  F64 rotateY;			// 旋转Y
  F64 rotateZ;			// 旋转Z
  F64 scale;			// 比例系数

  //U8  fileType;				// 文件类型,0是顺序排列的数据,1非顺序排列的数据,1一般用于扫描仪采集保存的临时数据
  //CHAR Reversed[114];			// 保留位
  CHAR Reversed[35];			// 保留位

  HLSheader()
  {
    clean_las_header();
  }
  
  //! 将内存流解析到文件头结构体
  inline void prase(char* hdData)
  {
	  memcpy(file_signature,hdData,4);
	  hdData += 4;

	  memcpy(&project_ID_GUID_data_1,hdData,4);
	  hdData += 4;
	  memcpy(&project_ID_GUID_data_2,hdData,2);
	  hdData += 2;
	  memcpy(&project_ID_GUID_data_3,hdData,2);
	  hdData += 2;
	  memcpy(project_ID_GUID_data_4,hdData,8);
	  hdData += 8;

	  memcpy(&file_source_id,hdData,2);
	  hdData += 2;
	  memcpy(&version_major,hdData,1);
	  hdData += 1;
	  memcpy(&version_minor,hdData,1);
	  hdData += 1;
	  memcpy(&system_identifier,hdData,32);
	  hdData += 32;
	  memcpy(generating_software,hdData,32);
	  hdData += 32;
	  memcpy(&file_creation_day,hdData,2);
	  hdData += 2;
	  memcpy(&file_creation_year,hdData,2);
	  hdData += 2;
	  memcpy(&header_size,hdData,2);
	  hdData += 2;

	  memcpy(&offset_to_point_data,hdData,4);
	  hdData += 4;
	  memcpy(&point_data_format,hdData,1);
	  hdData += 1;
	  memcpy(&point_data_record_length,hdData,2);
	  hdData += 2;
	  memcpy(&number_of_point_records,hdData,8);
	  hdData += 8;
	  memcpy(&number_of_row,hdData,4);
	  hdData += 4;
	  memcpy(&number_of_col,hdData,4);
	  hdData += 4;
	  //读取范围
	  memcpy(&max_x,hdData,4);
	  hdData += 4;
	  memcpy(&min_x,hdData,4);
	  hdData += 4;
	  memcpy(&max_y,hdData,4);
	  hdData += 4;
	  memcpy(&min_y,hdData,4);
	  hdData += 4;
	  memcpy(&max_z,hdData,4);
	  hdData += 4;
	  memcpy(&min_z,hdData,4);
	  hdData += 4;

	  //读取测站位置
	  memcpy(&centerX,hdData,4);
	  hdData += 4;
	  memcpy(&centerY,hdData,4);
	  hdData += 4;
	  memcpy(&centerZ,hdData,4);
	  hdData += 4;
	  //读取测站扫描仪旋转角度
	  memcpy(&yaw,hdData,4);
	  hdData += 4;
	  memcpy(&pitch,hdData,4);
	  hdData += 4;
	  memcpy(&roll,hdData,4);
	  hdData += 4;
	  //读取7参数
	  memcpy(&offsetX,hdData,8);
	  hdData += 8;
	  memcpy(&offsetY,hdData,8);
	  hdData += 8;
	  memcpy(&offsetZ,hdData,8);
	  hdData += 8;

	  memcpy(&rotateX,hdData,8);
	  hdData += 8;
	  memcpy(&rotateY,hdData,8);
	  hdData += 8;
	  memcpy(&rotateZ,hdData,8);
	  hdData += 8;

	  memcpy(&scale,hdData,8);
	  hdData += 8;

	  // 解决旧的数据没有7参数
	  if (scale == 0.0)
	  {
		  scale = 1.0;
	  }
  }
  //! 将文件头结构体,序列化到内存流
  inline void serialize(char* hdData) const
  {
	  memcpy(hdData,file_signature,4);
	  hdData += 4;

	  memcpy(hdData,&project_ID_GUID_data_1,4);
	  hdData += 4;
	  memcpy(hdData,&project_ID_GUID_data_2,2);
	  hdData += 2;
	  memcpy(hdData,&project_ID_GUID_data_3,2);
	  hdData += 2;
	  memcpy(hdData,project_ID_GUID_data_4,8);
	  hdData += 8;

	  memcpy(hdData,&file_source_id,2);
	  hdData += 2;
	  memcpy(hdData,&version_major,1);
	  hdData += 1;
	  memcpy(hdData,&version_minor,1);
	  hdData += 1;
	  memcpy(hdData,&system_identifier,32);
	  hdData += 32;
	  memcpy(hdData,generating_software,32);
	  hdData += 32;
	  memcpy(hdData,&file_creation_day,2);
	  hdData += 2;
	  memcpy(hdData,&file_creation_year,2);
	  hdData += 2;
	  memcpy(hdData,&header_size,2);
	  hdData += 2;

	  memcpy(hdData,&offset_to_point_data,4);
	  hdData += 4;
	  memcpy(hdData,&point_data_format,1);
	  hdData += 1;
	  memcpy(hdData,&point_data_record_length,2);
	  hdData += 2;
	  memcpy(hdData,&number_of_point_records,8);
	  hdData += 8;
	  memcpy(hdData,&number_of_row,4);
	  hdData += 4;
	  memcpy(hdData,&number_of_col,4);
	  hdData += 4;
	  //读取范围
	  memcpy(hdData,&max_x,4);
	  hdData += 4;
	  memcpy(hdData,&min_x,4);
	  hdData += 4;
	  memcpy(hdData,&max_y,4);
	  hdData += 4;
	  memcpy(hdData,&min_y,4);
	  hdData += 4;
	  memcpy(hdData,&max_z,4);
	  hdData += 4;
	  memcpy(hdData,&min_z,4);
	  hdData += 4;

	  //读取测站位置
	  memcpy(hdData,&centerX,4);
	  hdData += 4;
	  memcpy(hdData,&centerY,4);
	  hdData += 4;
	  memcpy(hdData,&centerZ,4);
	  hdData += 4;
	  //读取测站扫描仪旋转角度
	  memcpy(hdData,&yaw,4);
	  hdData += 4;
	  memcpy(hdData,&pitch,4);
	  hdData += 4;
	  memcpy(hdData,&roll,4);
	  hdData += 4;
	  //读取7参数
	  memcpy(hdData,&offsetX,8);
	  hdData += 8;
	  memcpy(hdData,&offsetY,8);
	  hdData += 8;
	  memcpy(hdData,&offsetZ,8);
	  hdData += 8;

	  memcpy(hdData,&rotateX,8);
	  hdData += 8;
	  memcpy(hdData,&rotateY,8);
	  hdData += 8;
	  memcpy(hdData,&rotateZ,8);
	  hdData += 8;

	  memcpy(hdData,&scale,8);
	  hdData += 8;
  }
  //! 根据7参数获取转换矩阵
  inline void computeMatrix(double* M)
  {
	  double cr = cos( rotateX );
	  double sr = sin( rotateX );
	  double cp = cos( rotateY );
	  double sp = sin( rotateY );
	  double cy = cos( rotateZ );
	  double sy = sin( rotateZ );

	  double a1,a2,a3,b1,b2,b3,c1,c2,c3;

	  a1 = cr * cy - sr * sp * sy; //cos(m_header.rotateX)*cos(m_header.rotateZ) - sin(m_header.rotateX)*sin(m_header.rotateY)*sin(m_header.rotateZ);
	  a2 = -cr * sy - sr * sp * cy;//-cos(m_header.rotateX)*sin(m_header.rotateZ) - sin(m_header.rotateX)*sin(m_header.rotateY)*cos(m_header.rotateZ);
	  a3 = -sr * cp;				 //-sin(m_header.rotateX)*cos(m_header.rotateY);
	  b1 = cp * sy;				 //cos(m_header.rotateY)*sin(m_header.rotateZ);
	  b2 = cp * cy;				 //cos(m_header.rotateY)*cos(m_header.rotateZ);
	  b3 = -sp;					 //-sin(m_header.rotateY);
	  c1 = sr * cy + cr * sp * sy; //sin(m_header.rotateX)*cos(m_header.rotateZ) + cos(m_header.rotateX)*sin(m_header.rotateY)*sin(m_header.rotateZ);
	  c2 = -sr * sy + cr * sp * cy;//-sin(m_header.rotateX)*sin(m_header.rotateZ) + cos(m_header.rotateX)*sin(m_header.rotateY)*cos(m_header.rotateZ);
	  c3 = cr * cp;				 //cos(m_header.rotateX)*cos(m_header.rotateY);
	  // 旋转矩阵
	  M[0] = a1; 
	  M[1] = a2; 
	  M[2] = a3;

	  M[4] = b1; 
	  M[5] = b2; 
	  M[6] = b3;

	  M[8] = c1; 
	  M[9] = c2; 
	  M[10] = c3;

	  // 偏移量
	  M[3] = offsetX;
	  M[7] = offsetY;
	  M[11] = offsetZ;

	  // 
	  M[12] = 0.0;
	  M[13] = 0.0;
	  M[14] = 0.0;
	  M[15] = scale;
  }
//
//  inline void getGlobalExtent(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax)
//  {
//	  double m[16];
//	  computeMatrix(m);
//
//	  // 底平面4个点
//	  double x1 = min_x,y1 = min_y,z1 = min_z;
//	  double x2 = max_x,y2 = min_y,z2 = min_z;
//	  double x3 = max_x,y3 = max_y,z3 = min_z;
//	  double x4 = min_x,y4 = max_y,z4 = min_z;
//	  // 顶平面4个点
//	  double x5 = min_x,y5 = min_y,z5 = max_z;
//	  double x6 = max_x,y6 = min_y,z6 = max_z;
//	  double x7 = max_x,y7 = max_y,z7 = max_z;
//	  double x8 = min_x,y8 = max_y,z8 = max_z;
//
//	  hdHomogeneousTransformPoint(m,x1,y1,z1);
//	  hdHomogeneousTransformPoint(m,x2,y2,z2);
//	  hdHomogeneousTransformPoint(m,x3,y3,z3);
//	  hdHomogeneousTransformPoint(m,x4,y4,z4);
//
//	  hdHomogeneousTransformPoint(m,x5,y5,z5);
//	  hdHomogeneousTransformPoint(m,x6,y6,z6);
//	  hdHomogeneousTransformPoint(m,x7,y7,z7);
//	  hdHomogeneousTransformPoint(m,x8,y8,z8);
//
//	  xmin = MIN(MIN(MIN(MIN(x1,x2),x3),x4),MIN(MIN(MIN(x5,x6),x7),x8));
//	  ymin = MIN(MIN(MIN(MIN(y1,y2),y3),y4),MIN(MIN(MIN(y5,y6),y7),y8));
//	  zmin = MIN(MIN(MIN(MIN(z1,z2),z3),z4),MIN(MIN(MIN(z5,z6),z7),z8));
//
//	  xmax = MAX(MAX(MAX(MAX(x1,x2),x3),x4),MAX(MAX(MAX(x5,x6),x7),x8));
//	  ymax = MAX(MAX(MAX(MAX(y1,y2),y3),y4),MAX(MAX(MAX(y5,y6),y7),y8));
//	  zmax = MAX(MAX(MAX(MAX(z1,z2),z3),z4),MAX(MAX(MAX(z5,z6),z7),z8));
//  }

  U8 get_pointformat() const
  {
	return point_data_format;
  }
  // 判断是按扫描列存储还是按块存储
  BOOL is_mesh() const
  {
	  return (file_signature[0] == 'M' && file_signature[1] == 'L' &&
		  file_signature[2] == 'S' && file_signature[3] == 'F');
  }

  inline U16 get_recordLength() const{return point_data_record_length;}

  void set_pointformat(U8 format = HLS_POINTFORMAT_XYZI)
  {
	  if ((format >= HLS_POINTFORMAT_XYZI && format <= HLS_POINTFORMAT_XYZIRGBP)
		  || (format >= HLS2_POINTFORMAT_RHVI && format <= HLS2_POINTFORMAT_XYZIRA))
	  {
		  if (format >= HLS_POINTFORMAT_XYZI && format <= HLS_POINTFORMAT_XYZIRGBP)
		  {
			  file_signature[0] = 'H'; file_signature[1] = 'L'; file_signature[2] = 'S'; file_signature[3] = 'F';
			  version_major = 1;
			  version_minor = 0;
		  }
		  else if (format >= HLS2_POINTFORMAT_RHVI && format <= HLS2_POINTFORMAT_XYZIRA)
		  {
			  file_signature[0] = 'H'; file_signature[1] = 'L'; file_signature[2] = 'S'; file_signature[3] = '2';
			  version_major = 2;
			  version_minor = 0;
		  }
		  point_data_format = format;
		  switch(format)
		  {
		  case HLS_POINTFORMAT_XYZI:
			  point_data_record_length = 16;
			  break;
		  case HLS_POINTFORMAT_RHVI:
			  point_data_record_length = 16;
			  break;
		  case HLS_POINTFORMAT_XYZIRGB:
			  point_data_record_length = 22;
			  break;
		  case HLS_POINTFORMAT_XYZIRGBP:
			  point_data_record_length = 31;
			  break;
		  case HLS2_POINTFORMAT_XYZ:
			  point_data_record_length = 12;
			  break;
		  case HLS2_POINTFORMAT_XYZI:
			  point_data_record_length = 14;
			  break;
		  case HLS2_POINTFORMAT_XYZIRGB:
			  point_data_record_length = 17;
			  break;
		  case HLS2_POINTFORMAT_XYZIRGBP:
			  point_data_record_length = 18;
			  break;
		  case HLS2_POINTFORMAT_XYZIRA:
			  point_data_record_length = 18;
			  break;
		  case HLS2_POINTFORMAT_RHVI:
			  point_data_record_length = 18;
			  break;
		  default:
			  break;
		  }
	  }
  }
  // set bounding box
  void set_bounding_box(F64 min_x, F64 min_y, F64 min_z, F64 max_x, F64 max_y, F64 max_z)
  {
	this->min_x = (F32)min_x;
    this->min_y = (F32)min_y;
    this->min_z = (F32)min_z;
    this->max_x = (F32)max_x;
    this->max_y = (F32)max_y;
    this->max_z = (F32)max_z;
  };

  // clean functions
  void clean_las_header()
  {
    memset((void*)this, 0, sizeof(HLSheader));
    file_signature[0] = 'H'; file_signature[1] = 'L'; file_signature[2] = 'S'; file_signature[3] = 'F';
    version_major = 1;
    version_minor = 0;
    header_size = 256;
    offset_to_point_data = 256;
    point_data_record_length = 16;
	scale = 1.0;
	// 默认极坐标
	point_data_format = HLS_POINTFORMAT_RHVI;	

	set_bounding_box(F32_MAX,F32_MAX,F32_MAX,F32_MIN,F32_MIN,F32_MIN);
  };

  void clean()
  {
    clean_las_header();
  };
  
  BOOL check() const
  {
    /*if (strncmp(file_signature, "HLSF", 4) != 0)
    {
      fprintf(stderr,"ERROR: wrong file signature '%s'\n", file_signature);
      return FALSE;
    }*/
    if ((version_major != 1) || (version_minor > 0))
    {
      fprintf(stderr,"WARNING: unknown version %d.%d (should be 1.0 or 1.1 or 1.2 or 1.3 or 1.4)\n", version_major, version_minor);
    }
    if (header_size < 256)
    {
      fprintf(stderr,"ERROR: header size is %d but should be at least 256\n", header_size);
      return FALSE;
    }
    if (offset_to_point_data < header_size)
    {
      fprintf(stderr,"ERROR: offset to point data %d is smaller than header size %d\n", offset_to_point_data, header_size);
      return FALSE;
    }
    if (max_x < min_x || max_y < min_y || max_z < min_z)
    {
      //fprintf(stderr,"WARNING: invalid bounding box [ %g %g %g / %g %g %g ]\n", min_x, min_y, min_z, max_x, max_y, max_z);
    }
    return TRUE;
  };

  ~HLSheader()
  {
    clean();
  };
};

#pragma  pack(pop)
}
#endif
