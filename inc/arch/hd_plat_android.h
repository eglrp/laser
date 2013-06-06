//----------------------------------------------------------------------------
//	hd_plat_android.h
//  Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------
#ifndef _HD_PLAT_ANDROID_H_
#define _HD_PLAT_ANDROID_H_ 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef ANDROID_OS

//----------------------------------------------------------------------------
// different windows operating system header files
// IMAC: MACRO ---> ANDROID_OS

#include <stdio.h>
#include <stdlib.h>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	this is for dll or com support
#define _inline_
#define __EXPORT__
#define __IMPORT__

#ifdef EMAP_DEV
#	ifdef EMAP_DEV_LIB
#	define DEV_EXPORT	__EXPORT__
#	else
#	define DEV_EXPORT	__IMPORT__
#	endif
#else
#	define DEV_EXPORT 
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	color macro
#define RGB_R(ulColor) ((e_uint8)(e_uint16)(e_int32)ulColor)
#define RGB_G(ulColor) ((e_uint16)(e_int32)ulColor >> 8)
#define RGB_B(ulColor) ((e_uint8)((e_int32)ulColor >> 16))

#if _HD_BYTE_ORDER_==_HD_BIG_EANDIN_
#define EMAP_RGB(r,g,b)	((e_uint32)(((e_uint8)(r)|			\
						((e_uint16)((e_uint8)(g))<<8))|	\
						(((e_uint32)(e_uint8)(b))<<16)))

#else
#define EMAP_RGB(r,g,b)	((e_uint32)(((e_uint8)(b)|			\
						((e_uint16)((e_uint8)(g))<<8))|	\
						(((e_uint32)(e_uint8)(r))<<16)))
#endif 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// std definitions of data type
typedef unsigned char		e_uint8;
typedef unsigned short int	e_uint16;
typedef unsigned long  int	e_uint32;			
typedef signed char			e_int8;
typedef signed short  int	e_int16;
typedef signed long	 int	e_int32;
typedef signed char			e_bool;
typedef	float				e_float32;
typedef	double				e_float64;
typedef signed long	long	e_int64;
typedef unsigned long long	e_uint64;
typedef	void*				e_handle;

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#ifdef USE_STDOUT_LOG //linux

#ifdef MSG_LOG2FILE
	#ifdef __cplusplus
		extern "C" DEV_EXPORT FILE *emap_ferr;
	#else
		extern DEV_EXPORT FILE *emap_ferr;
	#endif
	#define __stdout__ emap_ferr
#else
#define __stdout__ stdout
#endif
#define E_FPRINT fprintf


#else  //Android_OS
#include <android/log.h>
#ifdef MSG_LOG2FILE
	#ifdef __cplusplus
		extern "C" DEV_EXPORT FILE *emap_ferr;
	#else
		extern DEV_EXPORT FILE *emap_ferr;
	#endif
	#define __stdout__ emap_ferr
#else
#define __stdout__ ANDROID_LOG_INFO,__func__
#endif
#define E_FPRINT __android_log_print

#endif

//----------------------------------------------------------------------------
//string 

#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _snprintf	snprintf
#define _strdup		strdup

#pragma pack(2)
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

typedef struct tagRGBQUAD {
	e_uint8    rgbBlue;
	e_uint8    rgbGreen;
	e_uint8    rgbRed;
	e_uint8    rgbReserved;
} RGBQUAD;

typedef struct BITMAPINFOHEADER
{
	e_uint32	biSize;
	e_uint32	biWidth;
	e_uint32	biHeight;
	e_uint16	biPlanes;
	e_uint16	biBitCount;
	e_uint32	biCompression;
	e_uint32	biSizeImage;
	e_uint32	biXPelsPerMeter;
	e_uint32	biYPelsPerMeter;
	e_uint32	biClrUsed;
	e_uint32	biClrImportant;
}BITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[1];
} BITMAPINFO;

typedef struct BITMAPFILEHEADER
{
	e_uint16	bfType;
	e_uint32	bfSize;
	e_uint16	bfReserved1;
	e_uint16	bfReserved2;
	e_uint32	bfOffBits;
}BITMAPFILEHEADER;

#pragma pack()
//----------------------------------------------------------------------------
#endif

//----------------------------------------------------------------------------
//
#endif	//	_HD_PLAT_ANDROID_H_

//----------------------------------------------------------------------------
// EOF hd_plat_android.h
