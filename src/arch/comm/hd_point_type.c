/*!
 * \file hd_utils.h
 * \brief 
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#include <comm/hd_point_type.h>
#include <comm/hd_utils.h>

point_t*
malloc_points(e_int32 type, e_int32 size)
{
	point_t* ret = NULL;
	switch (type) {
	case PNT_TYPE_XYZ:
		ret = malloc(sizeof(point_t) + sizeof(point_xyz_t) * size);
		break;
	case PNT_TYPE_POLAR:
		ret = malloc(sizeof(point_t) + sizeof(point_polar_t) * size);
		break;
	case PNT_TYPE_GRAY:
		ret = malloc(sizeof(point_t) + sizeof(point_gray_t) * size);
		break;
	case PNT_TYPE_RGB:
		ret = malloc(sizeof(point_t) + sizeof(point_rgb_t) * size);
		break;
	default:
		return NULL;
	}
	ret->type = type;
	return ret;
}
void
free_points(point_t* pnts)
{
	if (pnts) {
		free(pnts);
	}
}

void points_polar2xyz(point_xyz_t *xyz, e_uint32 size, point_polar_t *polar) {
	int i;
	for (i = 0; i < size; i++) {
		hd_polar2xyz(&xyz[i].x, &xyz[i].y, &xyz[i].z,
		             polar[i].distance, polar[i].angle_h,polar[i].angle_v);
	}
}
