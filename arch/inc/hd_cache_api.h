//--------------------------------------------------------------------------------
// hd_cache_api.h
// uplusplus
// 2013-5-7
//
//
//--------------------------------------------------------------------------------

#ifndef _HD_CACHE_API_H_
#define _HD_CACHE_API_H_

#include <hd_plat_base.h>

//--------------------------------------------------------------------------------
//	global memory pool(in cache for fast accessing ...... )
//	1 memory
#define			CACHE_MEM_SIZE	(1024*1026*1)

//----------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	//	cache init/clear
	void 		DEV_EXPORT comm_cache_init( );
	void		DEV_EXPORT comm_cache_clear( );

	e_uint8		DEV_EXPORT *cache_global( );
	e_int32		DEV_EXPORT cache_size( );
	//----------------------------------------------------------------------------
	//thread safe?
	int			DEV_EXPORT cache_global_alloc_begin( );
	e_uint8		DEV_EXPORT *cache_global_alloc(int size,int num);
	void		DEV_EXPORT cache_global_alloc_end( );
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------
//内存申请用宏,连续申请
//usage:
//GLOBAL_MEM_ALLOC_BEGIN();
//GLOBAL_MEM_ALLOC(ind, int, n);
//GLOBAL_MEM_ALLOC(active, Edge, n);
//GLOBAL_MEM_ALLOC(range, int, circle_num);
//GLOBAL_MEM_ALLOC_END();
#define GLOBAL_MEM_ALLOC_BEGIN()  do{e_uint8* _global_cache_ptr_=cache_global()

#define GLOBAL_MEM_ALLOC(ptr, type, n) do{ \
	ptr = (type *)_global_cache_ptr_;  \
	_global_cache_ptr_+=sizeof(type)*n; }while(0)

#define GLOBAL_MEM_ALLOC_END() }while(0)
#endif	//	_HD_CACHE_API_H_
//--------------------------------------------------------------------------------
// EOF hd_cache_api.h
