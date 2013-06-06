//----------------------------------------------------------------------------
//	hd_plat_win32.h
//  Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------
#ifndef _HD_PLAT_WIN32_H_
#define _HD_PLAT_WIN32_H_ 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef MS_WIN32

//----------------------------------------------------------------------------
// different windows operating system header files
// MS-Windows: MACRO ---> MS_WIN32

#include <windows.h>
#pragma warning(once:4996) //仅显示一个

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	this is for dll or com support
#define _inline_
#define __EXPORT__ __declspec(dllexport)
#define __IMPORT__ __declspec(dllimport)

#ifdef E_DEV
#	ifdef E_DEV_LIB
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
#define RGB_A(ulColor) ((e_uint8)((e_int32)ulColor >> 24))

#if _HD_BYTE_ORDER_==_HD_BIG_EANDIN_
#define E_RGB(r,g,b)	((e_uint32)(((e_uint8)(r)|			\
						((e_uint16)((e_uint8)(g))<<8))|		\
						(((e_uint32)(e_uint8)(b))<<16)))

#define E_ARGB(a,r,g,b) ((e_uint32)(((e_uint8)(r)|			\
							((e_uint16)((e_uint8)(g))<<8))|		\
							(((e_uint32)(e_uint8)(b))<<16)))|	\
							(((e_uint32)(e_uint8)(b))<<24)))

#else
#define E_RGB(r,g,b)	((e_uint32)(((e_uint8)(b)|			\
						((e_uint16)((e_uint8)(g))<<8))|	\
						(((e_uint32)(e_uint8)(r))<<16)))

#define E_ARGB(a,r,g,b) ((e_uint32)(((e_uint8)(r)|			\
	((e_uint16)((e_uint8)(g))<<8))|		\
	(((e_uint32)(e_uint8)(b))<<16)))|	\
	(((e_uint32)(e_uint8)(b))<<24)))
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
typedef signed __int64		e_int64;
typedef unsigned __int64	e_uint64;
typedef	void*				e_handle;

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#ifdef MSG_LOG2FILE
	#ifdef __cplusplus
		extern "C" DEV_EXPORT FILE *zd_ferr;
	#else
		extern DEV_EXPORT FILE *zd_ferr;
	#endif
	#define __stdout__ zd_ferr
#else
#define __stdout__ stdout
#endif
#define E_FPRINT fprintf

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#endif

//----------------------------------------------------------------------------
//
#endif	//	_HD_PLAT_WIN32_H_

//----------------------------------------------------------------------------
// EOF hd_plat_win32.h
