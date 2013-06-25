//----------------------------------------------------------------------------
//	hd_plat_base.h
//  	uplusplus
// 	2013-5-7
//
//
//
//----------------------------------------------------------------------------
#ifndef _HD_PLAT_BASE_H_
#define _HD_PLAT_BASE_H_  

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	std c include files
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	defined different platform
#include "hd_plat_win32.h"
#include "hd_plat_android.h"

#include "hd_dbg_print.h"
#include "hd_mem_api.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef enum{
	HD_OS_UNSUPPORT=0xFF,
	HD_OS_MS_WIN32,
	HD_OS_MS_WINCE,
	HD_OS_SYMBIAN,
	HD_OS_MAC,
	HD_OS_IPHONE,
	HD_OS_ANDROID,
	HD_OS_ECOS,
	HD_OS_UCOS2,
	HD_OS_LINUX
}HD_OS_TYPE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	debug printf information macro
#define STDOUT __stdout__
#ifdef E_PRINT
	#define DMSG(_args_) E_FPRINT _args_
#else
	#define DMSG(_args_)
#endif

#define MAX_PATH_LEN 256

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	rect and point definition
typedef struct{
	e_int16 x;
	e_int16 y;
}sPoint_t;

typedef struct{
	e_int16 x;
	e_int16 y;
	e_int16 z;
}s3Point_t;

typedef struct{
	e_int32 x;
	e_int32 y;
}lPoint_t;

typedef struct{
	e_int32 x;
	e_int32 y;
	e_int32 z;
}l3Point_t;

typedef struct{
	e_int16 xmin;
	e_int16 ymin;
	e_int16 xmax;
	e_int16 ymax;
}sRect_t;

typedef struct{
	e_int32 xmin;
	e_int32 ymin;
	e_int32 xmax;
	e_int32 ymax;
}lRect_t;

typedef struct{
	e_float32 x;
	e_float32 y;
}f32Point_t;

typedef struct{
	e_float32 x;
	e_float32 y;
	e_float32 z;
}f323Point_t;

typedef struct{
	e_float32 xmin;
	e_float32 ymin;
	e_float32 xmax;
	e_float32 ymax;
}f32Rect_t;

typedef struct{
	e_float64 x;
	e_float64 y;
}f64Point_t;

typedef struct{
	e_float64 x;
	e_float64 y;
	e_float64 z;
}f643Point_t;

typedef struct{
	e_float64 xmin;
	e_float64 ymin;
	e_float64 xmax;
	e_float64 ymax;
}f64Rect_t;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// system alignment for data struct
#define E_ALIGNMENT 	4
#define ALIGN(_x_) 		(((_x_) + (E_ALIGNMENT-1)) & ~(E_ALIGNMENT-1))
#define max(a,b)    	(((a) > (b)) ? (a) : (b))
#define min(a,b)    	(((a) < (b)) ? (a) : (b))

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define EFLOAT_MIN			1E-20
#define EFLOAT_MAX			1E20
//#define ELONG_MAX			0x7FFFFFFF
//#define ELONG_MIN			0xFFFFFFFF
//#define ESHORT_MAX			0x00007FFF
//#define ESHORT_MIN			0x0000FFFF

#define ESHRT_MIN    (-32768)        /* minimum (signed) short value */
#define ESHRT_MAX      32767         /* maximum (signed) short value */
#define EUSHRT_MAX     0xffff        /* maximum unsigned short value */
#define EINT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#define EINT_MAX       2147483647    /* maximum (signed) int value */
#define EUINT_MAX      0xffffffff    /* maximum unsigned int value */
#define ELONG_MIN    (-2147483647L - 1) /* minimum (signed) long value */
#define ELONG_MAX      2147483647L   /* maximum (signed) long value */
#define EULONG_MAX     0xffffffffUL  /* maximum unsigned long value */
#define ELLONG_MAX     9223372036854775807i64       /* maximum signed long long int value */
#define ELLONG_MIN   (-9223372036854775807i64 - 1)  /* minimum signed long long int value */
#define EULLONG_MAX    0xffffffffffffffffui64       /* maximum unsigned long long int value */

//---------------------------------------------------------------------------
#define		ENT_MAX_PNT_NUM		9600
#define		ENT_MAX_UNIT_NUM	256 //最大单元个数(多圈的圈数,多点等的单元数)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// macro redefinition
#ifdef		NULL
#undef		NULL
#endif
#define		NULL						(0x0)

#ifdef		false
#undef		false
#endif
#define		false						0

#ifdef		true
#undef		true
#endif
#define		true						1

#ifdef		TRUE
#undef		TRUE
#endif
#define 	TRUE 						true

#ifdef		FALSE
#undef		FALSE
#endif
#define 	FALSE 						false

//----------------------------------------------------------------------------
//
#define REPLACE_SPLASH(_string_ptr_) do{	\
	char *p = _string_ptr_;	\
	while(*p)	\
	{	\
		if(*p == '\\') *p = '/'; \
		p++; \
	} \
}while(0)



//error definition
typedef enum E_RESULT{
	E_OK = 1,
	E_ERROR = 0,
	E_ERROR_TIME_OUT = -1,
	E_ERROR_IO = -2,
	E_ERROR_BAD_CHECKSUM = -3,
	E_ERROR_THREAD = -4,
	E_ERROR_CONFIG = -5,
	E_ERROR_BAD_ALLOCATE = -6,
	E_ERROR_INVALID_HANDLER = -7,
	E_ERROR_INVALID_CALL = -8,
	E_ERROR_INVALID_STATUS = -9,
	E_ERROR_INVALID_PARAMETER = -10,
	E_ERROR_INVALID_ADDRESS = -11,
/*RNADOM VALUE TO MARK INVALID INTEGER*/
	E_ERROR_INVALID_VALUE = -1760956276,
}E_RESULT;

//connection control
enum {
	E_TCP =1,
	E_UDP =2,
	E_BLOCK =3,
	E_NONBLOCK =4,
	E_READ =5,
	E_WRITE =6,
};


/****
	* 判断值是否为真,并退出当前函数
	* 典型用法：
	* ret = malloc(sizeof(int));
	* 对有返回值的函数：
	* e_assert(ret!=0, E_ERROR_BAD_ALLOCATE);
	* 对有无返回值的函数：
	* e_assert(ret!=0);
	*/
#define e_assert(_arg_,...) \
do{ \
	if(!(_arg_)){ \
		DMSG((STDOUT,"%s:%d:%s",__FILE__,__LINE__,#_arg_ " is false. Error " #__VA_ARGS__ "\r\n")); \
		return __VA_ARGS__;} \
}while(0)


/****
	* 判断值是否为真
	* 典型用法：
	* ret = func();
	* if(!e_check(ret,"func异常！")) goto E_OUT1;
	* 或：
	* if(!e_check(ret)) goto E_OUT1;
	*/
#define e_check(_arg_,...) \
({ \
	if((_arg_)) \
		DMSG((STDOUT,"%s:%d:%s",__FILE__,__LINE__,#_arg_ ":\t" #__VA_ARGS__ "\r\n")); \
	(_arg_); \
})


/****
	* 判断返回值是否失败
	* 典型用法：
	* ret = func();
	* if(e_failed(ret,"func异常！")) goto E_OUT1;
	* 或：
	* if(e_failed(ret)) goto E_OUT1;
	*/
#define e_failed(_arg_,...) \
({ \
	if((_arg_)<=0) \
		DMSG((STDOUT,"%s:%d:%s",__FILE__,__LINE__,#_arg_ " trace failed:\t" #__VA_ARGS__  "\r\n")); \
	(_arg_)<=0; \
})

//----------------------------------------------------------------------------
#endif	//	_HD_PLAT_BASE_H_

//----------------------------------------------------------------------------
// EOF hd_plat_base.h
