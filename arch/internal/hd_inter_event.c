//-----------------------------------------------------------------------------
//	file:		hd_inter_event.c
//	author: Joy.you
//	2013-05-08
//
//------------------------------------------------------------------------------

#include "hd_inter_event.h"

//---------------------------------------------------------------------
#ifdef DMSG
#undef DMSG
#define DMSG
#endif
//---------------------------------------------------------------------
//
static e_uint8	eventman_mem[EVENT_MEM_SIZE];
static event_man_t	eventman;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static e_uint8
eventman_init_mem_pool( event_man_t *module )
{
	e_uint8 *mem;
	e_uint32 avail_mem_size, data_mem_size;

	// int free list head
	INIT_LIST_HEAD(&module->free_list_head);
	
	// set
	mem					= module->mem;
	avail_mem_size		= module->size;
	data_mem_size		= ALIGN(sizeof(event_data_t));

	// dispose
	while (avail_mem_size >= data_mem_size ){
    event_data_t *data = (event_data_t *)mem;
	
		// add to list
    list_add_before(&data->list_node, &module->free_list_head);
        
    mem	+= data_mem_size;
    avail_mem_size	-= data_mem_size;
		module->max_num ++;
  }
	module->state = 1;
	return 1;
}

//---------------------------------------------------------------------
//
void
eventman_freedata( event_data_t *event, event_man_t *module )
{
  list_add_after(&event->list_node, &module->free_list_head);
}
 
//---------------------------------------------------------------------
// 
event_data_t *
eventman_allocdata( event_man_t *module )
{
  if ( !list_empty(module->free_list_head) ){
     event_data_t *newdata;
     newdata = list_entry(module->free_list_head.next, event_data_t, list_node);
     list_del(module->free_list_head.next);
     return newdata; 
  }
  else
     return NULL;
}

// --------------------------------------------------------------------
//
void
eventman_add_data( event_data_t *event, event_man_t *module )
{
	// insert data to list head
  list_add_after(&event->list_node, &module->used_list_head);
  module->cur_num++;
}

// --------------------------------------------------------------------
//
void
eventman_del_data( event_data_t *event, event_man_t *module )
{
	// delete data
  list_del(&event->list_node);
  module->cur_num--;
}

//---------------------------------------------------------------------
//
e_uint8
eventman_createmodule( e_uint8 *mem, e_int32 size, event_man_t *module )
{
	BZERO(module, event_man_t );

	if( size <= 0 )
		return 0;

	module->mem = mem;
	module->size = size;
	INIT_LIST_HEAD( &module->used_list_head );

	return eventman_init_mem_pool( module );
}

//---------------------------------------------------------------------
//
void
eventman_closemodule( event_man_t *module )
{
	BZERO(module, event_man_t );
	INIT_LIST_HEAD( &module->used_list_head );
	INIT_LIST_HEAD(&module->free_list_head);
}

//---------------------------------------------------------------------
//
e_uint8
eventman_modulestate( event_man_t *module )
{
	return module->state;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
e_uint8 
internal_eventman_init()
{
	DMSG((STDOUT,"internal_eventman_init, success\r\n"));
	mutex_init( &eventman.lock );
	return eventman_createmodule( eventman_mem, EVENT_MEM_SIZE, &eventman );
}

void
internal_eventman_exit()
{
	DMSG((STDOUT,"internal_eventman_exit, success\r\n"));
	mutex_destroy( &eventman.lock );
	eventman_closemodule( &eventman );	
}

event_data_t *
internal_eventman_alloc( )
{
	event_data_t *data;
	if( eventman.state==0 ){
		internal_eventman_init();
	}
	
	mutex_lock( &eventman.lock );
	data = eventman_allocdata( &eventman );
	if( data == 0x0 ){
		DMSG((STDOUT,"alloc eventman.cur_num: %d\r\n",eventman.cur_num));
		mutex_unlock( &eventman.lock );
		return 0x0;
	}

	eventman_add_data( data, &eventman );
	mutex_unlock( &eventman.lock );
	DMSG((STDOUT,"alloc eventman.cur_num: %d\r\n",eventman.cur_num));
	return data;
}

void 
internal_eventman_free( event_data_t *event )
{
	mutex_lock( &eventman.lock );

	eventman_del_data( event, &eventman );
	eventman_freedata( event, &eventman );

	mutex_unlock( &eventman.lock );
	DMSG((STDOUT,"free eventman.cur_num: %d\r\n",eventman.cur_num));
}
