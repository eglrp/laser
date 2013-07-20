/*
===============================================================================

  FILE:  mydefs.hpp
  
  CONTENTS:

    Basic data type definitions to be robust across platforms.
 
  PROGRAMMERS:
  
    martin.isenburg@gmail.com
  
  COPYRIGHT:

    (c) 2005-2011, Martin Isenburg, LASSO - tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    10 January 2011 -- licensing change for LGPL release and liblas integration
    13 July 2005 -- created after returning with many mosquito bites from OBX
  
===============================================================================
*/
#ifndef MYDEFS_HPP
#define MYDEFS_HPP

#ifdef HLS_EXPORTS
#define HLS_API
#else
#define HLS_API
#endif

#define UNORDERED 1

typedef char               CHAR;

typedef int                I32;
typedef short              I16;
typedef char               I8;

typedef unsigned int       U32;
typedef unsigned short     U16;
typedef unsigned char      U8;

#if defined(_WIN32)            // 64 byte integer under Windows 
typedef unsigned __int64   U64;
typedef __int64            I64;
#else                          // 64 byte integer elsewhere ... 
typedef unsigned long long U64;
typedef long long          I64;
#endif

typedef float              F32;
typedef double             F64;

#if defined(_MSC_VER)// && (_MSC_VER < 1300)
#pragma warning(disable:4996)
typedef int                BOOL;
#else
typedef bool               BOOL;
#endif

typedef union U32I32F32 { U32 u32; I32 i32; F32 f32; } U32I32F32;
typedef union U64I64F64 { U64 u64; I64 i64; F64 f64; } U64I64F64;

#undef F32_MAX
#define F32_MAX            +2.0e+37f
#undef F32_MIN
#define F32_MIN            -2.0e+37f

#undef F64_MAX
#define F64_MAX            +2.0e+307
#undef F64_MIN
#define F64_MIN            -2.0e+307

#undef U8_MIN
#define U8_MIN             ((U8)0x0)  // 0
#undef U8_MAX
#define U8_MAX             ((U8)0xFF) // 255
#undef U8_MAX_PLUS_ONE
#define U8_MAX_PLUS_ONE    0x0100     // 256

#undef U16_MIN
#define U16_MIN            ((U16)0x0)    // 0
#undef U16_MAX
#define U16_MAX            ((U16)0xFFFF) // 65535
#undef U16_MAX_PLUS_ONE
#define U16_MAX_PLUS_ONE   0x00010000    // 65536

#undef U32_MIN
#define U32_MIN            ((U32)0x0)            // 0
#undef U32_MAX
#define U32_MAX            ((U32)0xFFFFFFFF)     // 4294967295
#if defined(WIN32)            // 64 byte unsigned int constant under Windows 
#define U32_MAX_PLUS_ONE   0x0000000100000000    // 4294967296
#else                         // 64 byte unsigned int constant elsewhere ... 
#define U32_MAX_PLUS_ONE   0x0000000100000000ull // 4294967296
#endif

#undef I8_MIN
#define I8_MIN             ((I8)0x80) // -128
#undef I8_MAX
#define I8_MAX             ((I8)0x7F) // 127

#undef I16_MIN
#define I16_MIN            ((I16)0x8000) // -32768
#undef I16_MAX
#define I16_MAX            ((I16)0x7FFF) // 32767

#undef I32_MIN
#define I32_MIN            ((I32)0x80000000) // -2147483648
#undef I32_MAX
#define I32_MAX            ((I32)0x7FFFFFFF) //  2147483647

#undef I64_MIN
#define I64_MIN            ((I64)0x8000000000000000)
#undef I64_MAX
#define I64_MAX            ((I64)0x7FFFFFFFFFFFFFFF)

#undef U8_FOLD
#define U8_FOLD(n)      (((n) < U8_MIN) ? (n+U8_MAX_PLUS_ONE) : (((n) > U8_MAX) ? (n-U8_MAX_PLUS_ONE) : (n)))
#undef I8_CLAMP
#define I8_CLAMP(n)     (((n) <= I8_MIN) ? I8_MIN : (((n) >= I8_MAX) ? I8_MAX : ((I8)(n))))
#undef U8_CLAMP
#define U8_CLAMP(n)     (((n) <= U8_MIN) ? U8_MIN : (((n) >= U8_MAX) ? U8_MAX : ((U8)(n))))
#undef I16_CLAMP
#define I16_CLAMP(n)    (((n) <= I16_MIN) ? I16_MIN : (((n) >= I16_MAX) ? I16_MAX : ((I16)(n))))
#undef U16_CLAMP
#define U16_CLAMP(n)    (((n) <= U16_MIN) ? U16_MIN : (((n) >= U16_MAX) ? U16_MAX : ((U16)(n))))
#undef I32_CLAMP
#define I32_CLAMP(n)    (((n) <= I32_MIN) ? I32_MIN : (((n) >= I32_MAX) ? I32_MAX : ((I32)(n))))
#undef U32_CLAMP
#define U32_CLAMP(n)    (((n) <= U32_MIN) ? U32_MIN : (((n) >= U32_MAX) ? U32_MAX : ((U32)(n))))
#undef I8_QUANTIZE
#define I8_QUANTIZE(n) (((n) >= 0) ? (I8)((n)+0.5f) : (I8)((n)-0.5f))
#undef U8_QUANTIZE
#define U8_QUANTIZE(n) (((n) >= 0) ? (U8)((n)+0.5f) : (U8)(0))

#undef I16_QUANTIZE
#define I16_QUANTIZE(n) (((n) >= 0) ? (I16)((n)+0.5f) : (I16)((n)-0.5f))
#undef U16_QUANTIZE
#define U16_QUANTIZE(n) (((n) >= 0) ? (U16)((n)+0.5f) : (U16)(0))
#undef I32_QUANTIZE
#define I32_QUANTIZE(n) (((n) >= 0) ? (I32)((n)+0.5f) : (I32)((n)-0.5f))
#undef U32_QUANTIZE
#define U32_QUANTIZE(n) (((n) >= 0) ? (U32)((n)+0.5f) : (U32)(0))
#undef I64_QUANTIZE
#define I64_QUANTIZE(n) (((n) >= 0) ? (I64)((n)+0.5f) : (I64)((n)-0.5f))
#undef U64_QUANTIZE
#define U64_QUANTIZE(n) (((n) >= 0) ? (U64)((n)+0.5f) : (U64)(0))
#undef I16_FLOOR
#define I16_FLOOR(n) ((((I16)(n)) > (n)) ? (((I16)(n))-1) : ((I16)(n)))
#undef I32_FLOOR
#define I32_FLOOR(n) ((((I32)(n)) > (n)) ? (((I32)(n))-1) : ((I32)(n)))
#undef I64_FLOOR
#define I64_FLOOR(n) ((((I64)(n)) > (n)) ? (((I64)(n))-1) : ((I64)(n)))
#undef I16_CEIL
#define I16_CEIL(n) ((((I16)(n)) < (n)) ? (((I16)(n))+1) : ((I16)(n)))
#undef I32_CEIL
#define I32_CEIL(n) ((((I32)(n)) < (n)) ? (((I32)(n))+1) : ((I32)(n)))
#undef I64_CEIL
#define I64_CEIL(n) ((((I64)(n)) < (n)) ? (((I64)(n))+1) : ((I64)(n)))

#undef I8_FITS_IN_RANGE
#define I8_FITS_IN_RANGE(n) (((n) >= I8_MIN) || ((n) <= I8_MAX) ? TRUE : FALSE)
#undef U8_FITS_IN_RANGE
#define U8_FITS_IN_RANGE(n) (((n) >= U8_MIN) || ((n) <= U8_MAX) ? TRUE : FALSE)
#undef I16_FITS_IN_RANGE
#define I16_FITS_IN_RANGE(n) (((n) >= I16_MIN) || ((n) <= I16_MAX) ? TRUE : FALSE)
#undef U16_FITS_IN_RANGE
#define U16_FITS_IN_RANGE(n) (((n) >= U16_MIN) || ((n) <= U16_MAX) ? TRUE : FALSE)

#undef F32_IS_FINITE
#define F32_IS_FINITE(n) ((F32_MIN < (n)) && ((n) < F32_MAX))
#undef F64_IS_FINITE
#define F64_IS_FINITE(n) ((F64_MIN < (n)) && ((n) < F64_MAX))
#undef U32_ZERO_BIT_0
#define U32_ZERO_BIT_0(n) (((n)&(U32)0xFFFFFFFE))

#undef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))

#include <math.h>
__inline float
	hd_round (float number)
{
	return (number < 0.0f ? ceil(number - 0.5f) : floor(number + 0.5f));
}

__inline double
	hd_round (double number)
{
	return (number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5));
}

#define hd_lrint(x) ((long int) hd_round(x))
#define hd_lrintf(x) ((long int) hd_round(x))

#ifndef ERASE_ARRAY
#define ERASE_ARRAY(var, size) memset(var, 0, size*sizeof(*var))
#endif

#ifndef SET_ARRAY
#define SET_ARRAY(var, value, size) {for (int i=0; i<(int)size; ++i) var[i]=value;}
#endif

# define hd_isnan(x)    _isnan(x)
# define hd_isfinite(x) (_finite(x) != 0)
# define hd_isinf(x)    (_finite(x) == 0)

#ifndef DEG2RAD
#define DEG2RAD(x) ((x)*0.0174532925199433)
#endif

#ifndef RAD2DEG
#define RAD2DEG(x) ((x)*57.29577951308233)
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef NULL
#define NULL    0
#endif

const F32 ColorRCP = 1.0f/255.0f;

inline BOOL IS_LITTLE_ENDIAN()
{
  const U32 i = 1;
  return (*((U8*)&i) == 1);
}

#define ENDIANSWAP16(n) \
	( ((((U16) n) << 8) & 0xFF00) | \
	  ((((U16) n) >> 8) & 0x00FF) )

#define ENDIANSWAP32(n) \
	( ((((U32) n) << 24) & 0xFF000000) |	\
	  ((((U32) n) <<  8) & 0x00FF0000) |	\
	  ((((U32) n) >>  8) & 0x0000FF00) |	\
	  ((((U32) n) >> 24) & 0x000000FF) )

inline void ENDIAN_SWAP_16(U8* field)
{
  U8 help = field[0];
  field[0] = field[1];
  field[1] = help;
}

inline void ENDIAN_SWAP_32(U8* field)
{
  U8 help;
  help = field[0];
  field[0] = field[3];
  field[3] = help;
  help = field[1];
  field[1] = field[2];
  field[2] = help;
}

inline void ENDIAN_SWAP_64(U8* field)
{
  U8 help;
  help = field[0];
  field[0] = field[7];
  field[7] = help;
  help = field[1];
  field[1] = field[6];
  field[6] = help;
  help = field[2];
  field[2] = field[5];
  field[5] = help;
  help = field[3];
  field[3] = field[4];
  field[4] = help;
}

inline void ENDIAN_SWAP_16(const U8* from, U8* to)
{
  to[0] = from[1];
  to[1] = from[0];
}

inline void ENDIAN_SWAP_32(const U8* from, U8* to)
{
  to[0] = from[3];
  to[1] = from[2];
  to[2] = from[1];
  to[3] = from[0];
}

inline void ENDIAN_SWAP_64(const U8* from, U8* to)
{
  to[0] = from[7];
  to[1] = from[6];
  to[2] = from[5];
  to[3] = from[4];
  to[4] = from[3];
  to[5] = from[2];
  to[6] = from[1];
  to[7] = from[0];
}

inline int GetDayOfYear(int month,int day)
{
	static const int MonthDays[] = {31,59,90,120,151,181,212,243,273,304,334,365};
	if (month >= 1 && month <= 12)
	{
		if(month > 1)
		{
			if (day > (MonthDays[month-1] - MonthDays[month-2]))
			{
				return 0;
			}
			return MonthDays[month-2] + day;
		}
		else
		{
			return day;
		}
	}
	else
		return 0;
}
#endif
