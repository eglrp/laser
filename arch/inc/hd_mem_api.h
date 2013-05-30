//----------------------------------------------------------------------------
//	hd_mem_api.h
//  author: Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------

#ifndef _HD_MEM_API_H_
#define _HD_MEM_API_H_

#include <hd_plat_base.h>
//----------------------------------------------------------------------------
//	create memory in system or not
#define E_MEM_IN		0x00001
#define E_MEM_OUT		0x00002

//----------------------------------------------------------------------------
//	mem leak macro
#ifdef MEM_LEAK_DEBUG
#	define MEM_LEAK_START()				mem_start();
#	define MEM_LEAK_EXIT( _filename_ )	mem_exit( _filename_ );
#else
#	define MEM_LEAK_START( )
#	define MEM_LEAK_EXIT( _filename_ )
#endif

#ifdef MEM_LEAK_DEBUG
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
//	mem-leak malloc and free
void		DEV_EXPORT *mem_malloc( ems_int32 size, char *file, ems_uint32 line );
void		DEV_EXPORT mem_free( void *memblock );
void		DEV_EXPORT mem_memset( void *dst, int val, int count );

//	mem-leak reset and output
void		DEV_EXPORT mem_reset( );
void		DEV_EXPORT mem_output( char *filename );

//	mem-leak start and exit
void		DEV_EXPORT mem_start( );
void		DEV_EXPORT mem_exit( char *filename );
void		
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
#else
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
void		DEV_EXPORT *mem_malloc( e_int32 size );
void		DEV_EXPORT mem_free( void *memblock );
void		DEV_EXPORT mem_memset( void *dst, int val, int count );
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#endif


//----------------------------------------------------------------------------
//	redefine assert macro
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
//----------------------------------------------------------------------------
 
void DEV_EXPORT emap_assert(const char *file, const int line, const char *condition);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
	
//----------------------------------------------------------------------------
//	BZERO macro definition
#define BZERO( _va_, _type_ )		mem_memset((_va_),'\0',ALIGN(sizeof(_type_)))

//----------------------------------------------------------------------------
// memory malloc and free
#ifdef MEM_LEAK_DEBUG
#define E_MALLOC( _size_, _p_, _type_ )	\
	_p_ = (_type_*)mem_malloc(_size_,__FILE__,__LINE__);
	
#define E_FREE( _p_ )	\
	mem_free( _p_ );	\
	_p_ = NULL;
#else
#define E_MALLOC( _size_, _p_, _type_ )	\
	_p_ = (_type_*)mem_malloc(_size_);
	
#define E_FREE( _p_ )	\
	mem_free( _p_ );	\
	_p_ = NULL;
#endif

//----------------------------------------------------------------------------
//
#endif	//	_HD_MEM_API_H_
//----------------------------------------------------------------------------
//	EOF hd_mem_api.h



