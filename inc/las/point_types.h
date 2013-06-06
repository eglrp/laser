/*! point_types.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : point_types.h
相关文件     :
文件实现功能 : 点类型定义
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日 期        版本     修改人              修改内容
2012/05/31   1.0      龚书林
</PRE>
*******************************************************************************/

#ifndef __HD_POINT_TYPE_H_INCLUDED__
#define __HD_POINT_TYPE_H_INCLUDED__

// 设置结构体成员对齐1byte
#pragma  pack(push,1)

#include <math.h>
#include "hdDefs.h"

#ifndef SMR_RADIUS
#define SMR_RADIUS  0.0725
#endif

#ifndef CHB_SIDE
#define CHB_SIDE    0.1
#endif	

//#ifndef RAD2DEG
//#define RAD2DEG(x) ((x)*57.29578)
//#endif

namespace hd
{

	typedef enum ENUM_POINTTYPE
	{
		HDPCD_XYZ		= 1,	// XYZ属性
		HDPCD_INTENSITY = 2,	// 强度属性
		HDPCD_RGB       = 4,	// RGB属性
		HDPCD_CLASS		= 8,	// 分类属性
		HDPCD_NORMAL	= 16,	// NORMAL属性
	};
// 包含x,y,z属性的点
struct PointXYZ
{
	hd::f32  x,y,z;
	PointXYZ()
		:x(0.0f),y(0.0f),z(0.0f){}
	ENUM_POINTTYPE getType(){return HDPCD_XYZ;}
};

// 包含x,y,z,intensity属性的点
struct PointXYZI
{
	hd::f32  x,y,z;		// 坐标
	hd::u16 intensity;  // 反射强度,第16,15位分别标记选中,删除
	PointXYZI()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0){}

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY;}

	bool operator< (const struct PointXYZI& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZI& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}
	inline bool isValid()
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f);
	}
	inline bool isValid(int min,int max) 
	{
		return getIntensity() >= min && getIntensity() <= max;
	}

	// 获取反射强度
	inline hd::u16 getIntensity()const{return intensity & 0x3fff;}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}
};

// 点的RGB和属性分类
union PointRGBP
{
	struct
	{
		hd::u8 r;
		hd::u8 g;
		hd::u8 b;
		hd::u8 prop;
	};
	hd::u32 color_p;
};

// 包含x,y,z,r,g,b属性的点
struct PointXYZRGB
{
	hd::f32  x,y,z;	// 坐标
	hd::u8 b;
	hd::u8 g;
	hd::u8 r;
	PointXYZRGB()
		:x(0.0f),y(0.0f),z(0.0f),b(0),g(0),r(0){}//,_unused(0)
	int getType(){return HDPCD_XYZ | HDPCD_RGB;}
};

// 包含x,y,z,intensity,normal_x,normal_y,normal_z,curvature属性
struct PointXYZINormal
{
	hd::f32  x,y,z;		// 坐标
	hd::u16 intensity;		// 反射强度,第16,15位分别标记选中,删除
	hd::f32 nx,ny,nz;		// 法向量
	hd::f32 curvature;	// 曲率

	PointXYZINormal()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0),
		nx(0.0f),ny(0.0f),nz(0.0f),curvature(0.0f){}

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY | HDPCD_NORMAL;}

	bool operator< (const struct PointXYZINormal& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZINormal& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}
	inline bool isValid()
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f);
	}
	inline bool isValid(int min,int max) 
	{
		return getIntensity() >= min && getIntensity() <= max;
	}

	// 获取反射强度
	inline hd::u16 getIntensity()const{return intensity & 0x3fff;}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}
};

struct NormalPointXYZ
{
	hd::f32 x,y,z;
	hd::f32 nx,ny,nz;

	NormalPointXYZ()
		:x(0.0f),y(0.0f),z(0.0f),
		nx(0.0f),ny(0.0f),nz(0.0f){}

	int getType(){return HDPCD_XYZ | HDPCD_NORMAL;}
};
// 包含x,y,z,intensity,normal_x,normal_y,normal_z,curvature属性
struct Normal
{
	hd::f32 nx,ny,nz;		// 法向量
	hd::f32 curvature;	// 曲率

	Normal()
		:nx(0.0f),ny(0.0f),nz(0.0f),curvature(0.0f){}

	int getType(){return HDPCD_NORMAL;}
};


// 包含x,y,z,intensity,r,g,b属性的点
//template <typename T = f32>
struct PointXYZIRGB
{
	hd::f32  x,y,z;		// 坐标
	hd::u16 intensity;		// 反射强度,第16,15位分别标记选中,删除
	hd::u8 b;
	hd::u8 g;
	hd::u8 r;

	PointXYZIRGB()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0),b(0),g(0),r(0){}//,_unused(0)

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY | HDPCD_RGB;}

	bool operator< (const struct PointXYZIRGB& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZIRGB& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}
	inline bool isValid()
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f);
	}
	inline bool isValid(int min,int max) 
	{
		return getIntensity() >= min && getIntensity() <= max;
	}

	// 获取反射强度
	inline hd::u16 getIntensity()const{return intensity & 0x3fff;}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}
};

// 包含x,y,z,intensity,r,g,b,property属性的点
//template <typename T = f32>
struct PointXYZIPRGBA
{
	hd::f32  x,y,z;			// 坐标
	hd::u16 intensity;		// 反射强度,第16,15位分别标记选中,删除
	union
	{
		struct
		{
			hd::u8 r;
			hd::u8 g;
			hd::u8 b;
			hd::u8 prop;	// 属性分类
		};
		hd::u32 c_color;
	};

	//hd::u8 select;			// 选择状态,第4bit是1的话就是选中,第3bit是1的话就是删除
	PointXYZIPRGBA()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0),prop(0),b(0),g(0),r(0){}

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY | HDPCD_RGB | HDPCD_CLASS;}

	bool operator< (const struct PointXYZIPRGBA& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZIPRGBA& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}

	bool operator== (const struct PointXYZIPRGBA& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z &&
			getIntensity() == rhs.getIntensity() && prop == rhs.prop &&
			r == rhs.r && g == rhs.g && b == rhs.b;
	}

	//有效点判断  2012-07-24  weichi
	inline bool isValid()const 
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f) && !isDeleted();
	}
	inline bool isValid(int min,int max) const
	{
		return getIntensity() >= min && getIntensity() <= max && !isDeleted();
	}

	// 获取反射强度
	inline hd::u16 getIntensity() const{return (intensity & 0x3fff);}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置反选
	inline void setXorSelected()
	{
		intensity = (intensity ^ 0x8000);
	}
	// 设置取消选中
	inline void setUnSelected()
	{
		intensity = (intensity & 0x7fff);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}

	////判断点是否选中
	//inline bool isSelected()const
	//{
	//	return (select & 0x8) == 0x8;	// 选中点标记
	//}
	////判断点是否删除
	//inline bool isDeleted()const
	//{
	//	return (select & 0x4) == 0x4;	// 删除点标记
	//}
};

// 包含x,y,z,intensity属性的双精度坐标点
struct PointXYZI_D
{
	hd::f64  x,y,z;		// 坐标
	hd::u16 intensity;		// 反射强度
	PointXYZI_D()
		:x(0.0),y(0.0),z(0.0),intensity(0){}

	bool operator< (const struct PointXYZI_D& rhs) const
	{
		return intensity<rhs.intensity;
	}

	bool operator> (const struct PointXYZI_D& rhs) const
	{
		return intensity>rhs.intensity;
	}
	inline bool isValid()
	{
		return !(x == 0.0 && y == 0.0 && z == 0.0);
	}
	inline bool isValid(int min,int max) 
	{
		return intensity >= min && intensity <= max;
	}
};

/*
靶球拟合
*/
struct PointSphere
{
	hd::f32	rowScale,colScale;	// 靶球所在点云行列比例
	hd::f32	x,y,z;				// 靶球中心位置
	hd::s32		fitPtCount;		// 参与拟合的点数
	hd::f32	stdDev;				// 半径标准差
	PointSphere()
		:x(0.0f),y(0.0f),z(0.0f),rowScale(0.0f),colScale(0.0f),stdDev(0.0f),fitPtCount(0){}
};
/*
棋盘格标靶点
*/
struct PointChessboard
{
	hd::f32 rowScale,colScale;      //标靶所在的灰度图像素比例
	hd::s32 row,col;				  //标靶中心点所在的灰度图像行列号
	hd::f32 x,y,z;				  //标靶中心点三维坐标
	PointChessboard()
		:x(0.0f),y(0.0f),z(0.0f),rowScale(0.0f),colScale(0.0f),row(0),col(0) {}
};

/*
点云中的点分类
*/
typedef enum ENUM_POINTCLASS {
	CLASS_ALL = 0,
	CLASS_FIRST,
	CLASS_LAST,
	CLASS_GROUND,
	CLASS_OBJECT,
	CLASS_BUILDING,
	CLASS_VEGETATION,
	CLASS_MASS_POINTS,
	CLASS_WATER,
	CLASS_UNCLASSIFIED,
	CLASS_OVERLAP
} ENUM_POINTCLASS;

typedef enum ENUM_RENDERSTYLE {
	RENDER_BY_DEFAULT = 0,			//默认的用default显示
	RENDER_BY_RGB,				    //根据点云文件中的RGB显示
	RENDER_BY_INTENSITY,			//根据反射强度渲染
	RENDER_BY_Z,					//根据Z值渲染
	RENDER_BY_X,					//根据X值渲染
	RENDER_BY_Y,					//根据Y值渲染
	RENDER_BY_DISTANCE,				//根据距离渲染
	RENDER_BY_CLASS,				//按分类渲染
	RENDER_BY_FEATURE_BIN,			//按照分类实时渲染
	RENDER_BY_COL,                  //根据选择列范围渲染
} ENUM_RENDERSTYLE;

typedef enum ENUM_SHOWSTYLE {
	SHOW_ALL = 0,			//显示所有
	SHOW_SELECT,			//显示选择
	SHOW_UNSELECT,			//显示未选择
} ENUM_SHOWSTYLE;

// 全景与点云配准控制点
struct PanoControlPoint 
{
public:
	PanoControlPoint()
	{
		memset(this,0,sizeof(PanoControlPoint));
	}
	PanoControlPoint(int imgx,int imgy,f32 px,f32 py,f32 pz)
		:imageX(imgx),imageY(imgy),X(px),Y(py),Z(pz)
	{
	}
	int imageX;
	int imageY;
	hd::f32 X;
	hd::f32 Y;
	hd::f32 Z;
};

//! 获取扫描点的水平角angleX和垂直角angleZ
inline void GetPointAngle(double x,double y,double z,double& angleX,double& angleZ)
{
	angleZ = atan2(z,sqrt(x*x + y*y)) * 57.29577951308233;
	angleX = atan2(y,x) * 57.29577951308233;
}

}
//恢复默认结构体对齐
#pragma  pack(pop)

#endif
