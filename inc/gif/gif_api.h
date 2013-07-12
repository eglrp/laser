//--------------------------------------------------------------------------------
// hd_gif_api.h
// uplusplus
// 2013-6-28
//
//
//--------------------------------------------------------------------------------

#ifndef _HD_GIF_API_H_
#define _HD_GIF_API_H_

#include <arch/hd_plat_base.h>

typedef struct gif_t {
	size_t handle;
	e_int32 mode;
	e_int32 state;
	e_uint32 width, height;
	e_uint8 file_name[MAX_PATH_LEN];
} gif_t;

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
e_int32 DEV_EXPORT gif_open(gif_t * gif, char *file_name, e_uint32 width, e_uint32 height,
		e_int32 mode);
e_int32 DEV_EXPORT gif_close(gif_t * gif);
e_int32 DEV_EXPORT gif_put_header(gif_t * gif);
e_int32 DEV_EXPORT gif_put_image(gif_t * gif);
e_int32 DEV_EXPORT gif_put_scan_line(gif_t * gif, e_uint8* line);
e_int32 DEV_EXPORT gif_union(gif_t *gif, gif_t *gif1, gif_t *gif2);
e_int32 DEV_EXPORT gif_append(gif_t * gif1, gif_t * gif2);

#ifdef __cplusplus
}
#endif

#endif	//	_HD_GIF_API_H_
//--------------------------------------------------------------------------------
// EOF hd_gif_api.h
