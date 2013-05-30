//-----------------------------------------------------------------------------
//	file:	hd_inter_event.h
//	author: Joy.you
//	2013-05-08
//
//------------------------------------------------------------------------------

#ifndef _HD_INTER_EVENT_H_
#define _HD_INTER_EVENT_H_

//------------------------------------------------------------------------------
#include <hd_plat_base.h>
#include <hd_thread_api.h>
#include <hd_list.h>

//---------------------------------------------------------------------
#define EVENT_MEM_SIZE		4096	//([4096/24]=170)

//---------------------------------------------------------------------
//	event item data type definition
typedef struct{
	e_uint32	sys_msg;
	e_uint32	usr_msg;
	e_uint32	wparam;
	e_uint32	lparam;
	struct list_head list_node;
}event_data_t;

typedef event_data_t *event_data_ref; 
//------------------------------------------------------------------------------
//	event manager data type definition
typedef struct{
	e_uint8 state;
	e_int32	cur_num;
	e_int32 max_num;
	struct list_head free_list_head;
	struct list_head used_list_head;
	
	e_uint8	*mem;
	e_int32	size;
	mutex_t lock;
}event_man_t;

//------------------------------------------------------------------------------
//	event_entry - Assuming you have a struct of type _type_ that contains a
//	list which has the name _member_ in that struct type, then given the
//	address of that list in the struct, _event_, this returns the address
//	of the container structure */

#define event_entry( _event_, _type_, _member_ ) \
    ((_type_ *)((char *)(_event_)-(char *)&(((_type_*)0)->_member_)))

//---------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//---------------------------------------------------------------------
//
e_uint8 
internal_eventman_init();

void
internal_eventman_exit();

event_data_t *
internal_eventman_alloc( );

void 
internal_eventman_free( event_data_t *event );

//---------------------------------------------------------------------
//	create/close/state module
e_uint8
eventman_createmodule( e_uint8 *mem, e_int32 size, event_man_t *module );

void
eventman_closemodule( event_man_t *module );

e_uint8
eventman_modulestate( event_man_t *module );

//	alloc and free data
void
eventman_freedata( event_data_t *event, event_man_t *module );

event_data_t *
eventman_allocdata( event_man_t *module );
//	add data and delete data
void
eventman_add_data( event_data_t *event, event_man_t *module );

void
eventman_del_data( event_data_t *event, event_man_t *module );
//---------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------
//

#endif	//	_HD_INTER_EVENT_H_
//	EOF: hd_inter_event.h
