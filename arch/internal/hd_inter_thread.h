//-----------------------------------------------------------------------------
//	file:		hd_inter_thread.h
//	author: Joy.you
//	2013-05-08
//
//------------------------------------------------------------------------------

#ifndef _HD_INTER_THREAD_H_
#define _HD_INTER_THREAD_H_

//------------------------------------------------------------------------------


#include <hd_plat_base.h>
#include <hd_thread_api.h>

//------------------------------------------------------------------------------
//	thread manage data type definition

typedef struct{
	e_uint8	state;
	e_int32	cur_num;
	struct list_head used_list_head;
}thread_man_t;

#ifdef __cplusplus
extern "C"
#endif

thread_man_t g_thread_man;

#ifdef __cplusplus
}
#endif
#endif	//	_HD_INTER_THREAD_H_
//	EOF: hd_inter_thread.h
