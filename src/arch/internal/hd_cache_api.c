//--------------------------------------------------------------------------------
// hd_cache_api.c
// Joy.you
// 2013-05-08
//
//
//--------------------------------------------------------------------------------

#include <arch/hd_cache_api.h>


//--------------------------------------------------------------------------------
// global cache memory 
static e_uint8	*shared_mem_cache = 0x0;
static e_uint8	*shared_mem_cache_cur = 0x0;

//	cache init/clear
void 
comm_cache_init()
{
	shared_mem_cache = (e_uint8*)malloc(CACHE_MEM_SIZE);
}

void 
comm_cache_clear()
{
	free(shared_mem_cache);
	shared_mem_cache = 0x0;
}

e_uint8*
cache_global( )
{
	return shared_mem_cache;
}

e_int32		
cache_size( )
{
	return CACHE_MEM_SIZE;
}

//check if global cache is ready
int		 
cache_global_alloc_begin( )
{
	if(!shared_mem_cache||shared_mem_cache_cur) 
		return 0;
	else 
	{
		shared_mem_cache_cur = shared_mem_cache;
		return 1;
	}
}

//allocate a mem block,and shift cur
e_uint8*
cache_global_alloc(int size,int num)
{
	e_uint8 *ret=NULL;
	if(shared_mem_cache_cur>shared_mem_cache+CACHE_MEM_SIZE){
		return NULL;
	}
	ret = shared_mem_cache_cur;
	shared_mem_cache_cur += size*num;
	return ret;
}

//leave global cache
void		
cache_global_alloc_end( )
{
	shared_mem_cache_cur = 0x0;
}

//--------------------------------------------------------------------------------
// EOF hd_cache_api.c
