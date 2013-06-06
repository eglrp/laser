/*! hlsDefinition.h
********************************************************************************
<PRE>
模块名       : LASLib
文件名       : hl2Definition.h
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

#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "mydefs.hpp"
//定义车载扫描仪点格式
//#define HLS2_POINTFORMAT_XYZI		11		// 车载xyzi点云	


//定义HL2格式版本号
#define HL2FORMAT_VERSION_MAJOR 1
#define HL2FORMAT_VERSION_MINOR 0

namespace hd
{
#pragma  pack(push,1)
	
	// 记录圈数据无效点信息
	class HLS2_LOOPSEG
	{
	private:
		// 起始有效点
		U8 startHigh;
		U16 startLow;
		// 有效点个数
		U8 countHight;
		U16 countLow;
	public:
		HLS2_LOOPSEG()
			:startHigh(0),startLow(0),countHight(0),countLow(0){}
		
		inline void setStart(U32 start)
		{
			startLow = start;
			startHigh = start >> 16;
		}

		inline U32 getStart() const
		{
			return (startHigh << 16) | startLow;
		}

		inline void setCount(U32 count)
		{
			countLow = count;
			countHight = count >> 16;
		}

		inline U32 getCount() const
		{
			return (countHight << 16) | countLow;
		}
	};

	// 圈数据索引
	struct HLS2_LOOPINDEX
	{
		HLS2_LOOPINDEX()
			:offset(0),count(0),rowCount(0),segCount(0),fileNo(0),//rgbpFileNo(0),
			xmin(F32_MAX),ymin(F32_MAX),zmin(F32_MAX),
			xmax(F32_MIN),ymax(F32_MIN),zmax(F32_MIN),
			hasRGBP(1),res(0),minIntensity(0xffff),maxIntensity(0)
		{
			memset(headerRes,0,sizeof(29));
		}

		inline void ResetExtent()
		{
			xmin = ymin = zmin = F32_MAX;
			xmax = ymax = zmax = F32_MIN;
		}
		inline U16 GetPtSize(){return hasRGBP?18:14;}
		// 数据起始位置
		U64 offset;
		// 有效点个数
		U32 count;
		// 当前圈总共点数
		U32 rowCount;
		// 有效点区段数
		U16 segCount;

		U8 hasRGBP:1;		// 是否包含RGB属性
		U8 res:7;			// 保留位

		U16 fileNo;			// xyzi数据文件编号
		//U16 rgbpFileNo;		// r\g\b\p数据文件编号
		
		F32 xmin;
		F32 ymin;
		F32 zmin;
		F32 xmax;
		F32 ymax;
		F32 zmax;	

		U16 minIntensity;
		U16 maxIntensity;

		U8 headerRes[31];	// 保留位29,总字节数80
	};
	
	// 圈索引及无效点区段	
	class CLoopIndex
	{
	public:
		CLoopIndex()
		:m_pLoopSeg(NULL){}
		~CLoopIndex()
		{
			Clear();
		}
		void Clear()
		{
			if(m_pLoopSeg)
			{
				delete[] m_pLoopSeg;
				m_pLoopSeg = NULL;
			}
		}
		void AllocLoopSeg()
		{
			if(m_loopIdx.segCount > 0)
			{
				Clear();
				m_pLoopSeg = new HLS2_LOOPSEG[m_loopIdx.segCount];
			}
		}
		// 圈索引
		HLS2_LOOPINDEX m_loopIdx;
		// 圈数据无效点区段
		HLS2_LOOPSEG*  m_pLoopSeg;
	};
#pragma  pack(pop)
}
