//-----------------------------------------------------------------------------
//	hd_point_type.h
//  Joy.you
//  2013/07/05
//	
//-----------------------------------------------------------------------------


#ifndef _HD_POINT_H_
#define _HD_POINT_H_

/* Dependencies */
#include <arch/hd_plat_base.h>
/*结构体定义*/

enum {
	PNT_TYPE_NONE = 0,
	PNT_TYPE_XYZ,
	PNT_TYPE_POLAR,
	PNT_TYPE_GRAY,
	PNT_TYPE_RGB,
};

typedef struct point_xyz_t {
	e_float32 x;
	e_float32 y;
	e_float32 z;
	e_uint16 intensity;
} point_xyz_t;

typedef struct point_polar_t {
	e_float64 distance;
	e_float32 angle_h;
	e_float32 angle_v;
	e_uint16 intensity;
} point_polar_t;

typedef struct {
	e_uint8 r,g,b;
}point_rgb_t;

typedef struct {
	e_uint8 gray;
}point_gray_t;

#pragma pack(4)
typedef struct
{
	e_int32 type;
	e_uint8 mem[];
} point_t;
#pragma pack()

/*接口定义*/
#ifdef __cplusplus
extern "C"
{
#endif

point_t* DEV_EXPORT malloc_points(e_int32 type,e_int32 size);
void DEV_EXPORT free_points(point_t* pnts);
void DEV_EXPORT points_polar2xyz(point_xyz_t *xyz,e_uint32 size,point_polar_t *polar);

#ifdef __cplusplus
}
#endif

/* -----------------------------------------------------------------------*/
#endif /* #ifndef _HD_POINT_H_ */
/* EOF hd_list.h */
