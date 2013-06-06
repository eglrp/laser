/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCore
文件名		：hdDefs.h
相关文件	：
文件实现功能：定义基本数据类型。
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/2/27	1.0			杨峰		创建
</PRE>
******************************************************************************************************/

#pragma once
#include <wchar.h>

namespace hd
{
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	// Defines for s{w,n}printf because these methods do not match the ISO C
	// standard on Windows platforms, but it does on all others.
	// These should be int snprintf(char *str, size_t size, const char *format, ...);
	// and int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
	// Compiler version defines: VC6.0 : 1200, VC7.0 : 1300, VC7.1 : 1310, VC8.0 : 1400
#if defined(_MSC_VER) && _MSC_VER > 1310 && !defined (_WIN32_WCE)
#define swprintf swprintf_s
#define snprintf sprintf_s
#else
#define swprintf _snwprintf
#define snprintf _snprintf
#endif
#endif

	//! 8 bit unsigned variable.
	/** This is a typedef for unsigned char, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef unsigned __int8		u8;
#else
	typedef unsigned char		u8;
#endif

	//! 8 bit signed variable.
	/** This is a typedef for signed char, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef __int8			s8;
#else
	typedef signed char		s8;
#endif

	//! 8 bit character variable.
	/** This is a typedef for char, it ensures portability of the engine. */
	typedef char			c8;

	//! 16 bit unsigned variable.
	/** This is a typedef for unsigned short, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef unsigned __int16	u16;
#else
	typedef unsigned short		u16;
#endif

	//! 16 bit signed variable.
	/** This is a typedef for signed short, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef __int16			s16;
#else
	typedef signed short		s16;
#endif

	//! 32 bit unsigned variable.
	/** This is a typedef for unsigned int, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef unsigned __int32	u32;
#else
	typedef unsigned int		u32;
#endif

	//! 32 bit signed variable.
	/** This is a typedef for signed int, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef __int32			s32;
#else
	typedef signed int		s32;
#endif

#if defined(_WIN32)            // 64 byte integer under Windows 
typedef unsigned __int64   u64;
typedef __int64            s64;
#else                          // 64 byte integer elsewhere ... 
typedef unsigned long long u64;
typedef long long          s64;
#endif

	// 64 bit signed variable.
	// This is a typedef for __int64, it ensures portability of the engine.
	// This type is currently not used by the engine and not supported by compilers
	// other than Microsoft Compilers, so it is outcommented.
	//typedef __int64				s64;

	//! 32 bit floating point variable.
	/** This is a typedef for float, it ensures portability of the engine. */
	typedef float				f32;

	//! 64 bit floating point variable.
	/** This is a typedef for double, it ensures portability of the engine. */
	typedef double				f64;

	/*typedef int                I32;
	typedef short              I16;
	typedef char               I8;*/

}

//! WIN32 for Windows32
//! WIN64 for Windows64
// The windows platform and API support SDL and WINDOW device
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#define _HD_WINDOWS_
#define _HD_WINDOWS_API_
#define _HD_COMPILE_WITH_WINDOWS_DEVICE_
#endif


//! define a break macro for debugging.
#if defined(_DEBUG)
#if defined(_HD_WINDOWS_API_) && defined(_MSC_VER) && !defined (_WIN32_WCE)
#if defined(WIN64) || defined(_WIN64) // using portable common solution for x64 configuration
#include <crtdbg.h>
#define _HD_DEBUG_BREAK_IF( _CONDITION_ ) if (_CONDITION_) {_CrtDbgBreak();}
#else
#define _HD_DEBUG_BREAK_IF( _CONDITION_ ) if (_CONDITION_) {_asm int 3}
#endif
#else
#include "assert.h"
#define _HD_DEBUG_BREAK_IF( _CONDITION_ ) assert( !(_CONDITION_) );
#endif
#else
#define _HD_DEBUG_BREAK_IF( _CONDITION_ )
#endif