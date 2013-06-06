//----------------------------------------------------------------------------
//	hd_mem_file_android.c
//  Author: Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------

#ifdef ANDROID_OS

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include <arch/hd_plat_base.h>
#include <arch/hd_mem_api.h>

//----------------------------------------------------------------------------
//
#ifdef MEM_LEAK_DEBUG
#include <arch/hd_file_api.h>
#include <arch/hd_list.h>

//----------------------------------------------------------------------------
//	max node in module
#define MEM_MAXSTORE 1024*32

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	mem module
typedef struct{
	e_uint8	state;
	e_uint8	*mem;					//	memory pool address
	e_uint32	size;					//	memory pool size
	e_uint32	used_item;				//	node current number in memory pool
	e_uint32	free_item;				//	node max number in memory pool
	struct list_head used_list_head;	//	used list
	struct list_head free_list_head;	//	free list
}mem_module_t;

//	module item struct definition
typedef struct{
	e_uint32 call_addr;			//	call address
	e_uint32 alloc_addr;			//	alloc address
	e_uint32 size;				//	memory block size
	e_uint32 line;				//	memory allocate line in file
	
	char file[128];			//	file name
	struct list_head list_node;		//	node 
}mem_node_t;

//----------------------------------------------------------------------------
mem_module_t mem_leak_module;

//----------------------------------------------------------------------------

static void
mem_init_pool(mem_module_t *module)
{
    e_uint8  *mem;
    e_int32  avail_mem_size,node_mem_size;

    mem      = module->mem;
    avail_mem_size = module->size;
    node_mem_size = ALIGN(sizeof(mem_node_t));
    
    while (avail_mem_size >= node_mem_size)
    {
        mem_node_t *node = (mem_node_t *)mem;

        list_add_before(&node->list_node, &module->free_list_head);
        
        mem      += node_mem_size;
        avail_mem_size -= node_mem_size;
		module->free_item++;
    }
}

//---------------------------------------------------------------------
//
static void
mem_list_freedata( mem_module_t *module, mem_node_t *data )
{
  list_add_after(&data->list_node, &module->free_list_head);
  module->free_item++;
  module->used_item--;
}

//---------------------------------------------------------------------
// 
static mem_node_t *
mem_list_allocdata( mem_module_t *module )
{
  if ( !list_empty(module->free_list_head) ){
     mem_node_t *newdata;
     newdata = list_entry(module->free_list_head.next, mem_node_t, list_node);
     list_del(module->free_list_head.next);
	 module->free_item--;
	 module->used_item++;
     return newdata; 
  }
  else
     return NULL;
}

// --------------------------------------------------------------------
//
static void
mem_list_adddata( mem_module_t *module, mem_node_t *data )
{
	// insert data to list head
	list_add_after(&data->list_node, &module->used_list_head);
}

// --------------------------------------------------------------------
//
static void
mem_list_deldata( mem_module_t *module, mem_node_t *data )
{
	// delete data
  list_del(&data->list_node);
}

static mem_node_t *
mem_list_finddata( mem_module_t *module, e_uint32 alloc_addr )
{
	mem_node_t *data;
	struct list_head *node, *head;
	node = module->used_list_head.next;
	head = &module->used_list_head;
	while( node!= head ){
		data = list_entry(node, mem_node_t, list_node);
		if( data->alloc_addr==alloc_addr ){
			return data;
		}
		node = node->next;
	}
	
    return NULL;
}

//----------------------------------------------------------------------------

e_uint8
mem_createmodule( mem_module_t *module )
{
	e_uint8 *mem;
	//	1 init parameters
	//	2 alloc memory pool
	//	init parameters
	BZERO(module, mem_module_t );
	INIT_LIST_HEAD( &module->used_list_head );
	INIT_LIST_HEAD( &module->free_list_head );

	//	alloc memory pool
	module->size = ALIGN(sizeof(mem_node_t))*MEM_MAXSTORE;
	mem = (e_uint8*)malloc(module->size);
	if( mem == NULL ){
		DMSG((STDOUT,"failed to malloc memory\r\n"));
		return 0;
	}
	
	module->state = 1;
	module->mem = mem;

	mem_init_pool(module);
	DMSG((STDOUT,"success to create mem-leak module,mem mallocated!\r\n"));
	return 1;
}

//----------------------------------------------------------------------------
e_uint8
mem_modulestate( mem_module_t *module )
{
	return module->state;
}

//----------------------------------------------------------------------------

void
mem_closemodule( mem_module_t *module )
{
	if( !mem_modulestate(module)){
		DMSG((STDOUT,"module dose not been created\r\n"));
		return;
	}

	free(module->mem);
	BZERO(module, mem_module_t );
	DMSG((STDOUT,"success to close mem-leak module,mem released!\r\n"));
}


//----------------------------------------------------------------------------

e_uint8
mem_appendnode( mem_module_t *module, e_uint32 call_addr, e_uint32 alloc_addr,
			    e_uint32 size, char *file, e_uint32 line )
{
	mem_node_t *data;
	//	check module state and max number state
	if(!mem_modulestate(module)){
		DMSG((STDOUT,"module dose not been created \r\n"));
		return 0;
	}

	if(module->used_item>=MEM_MAXSTORE){
		DMSG((STDOUT,"have been max number, failed \r\n"));
		return 0;
	}
	
	//	1 allocate a list and insert
	//	2 set value and insert to list head

	// allocate a list and insert
	data = mem_list_allocdata( module );
	if( NULL == data ){
		// dost not have enough memory
		return 0;
	}

	// set key and value
	data->alloc_addr = alloc_addr;
	data->call_addr = call_addr;
	data->line = line;
	data->size = size;
	sprintf( data->file, "%s", file );
	
	// insert to list head
	mem_list_adddata( module, data );
	return 1;
}

void
mem_deletenode( mem_module_t *module, e_uint32 alloc_addr )
{
	mem_node_t *data;

	//	check module state and max number state
	if(!mem_modulestate(module)){
		DMSG((STDOUT,"module dose not been created \r\n"));
		return;
	}

	if(alloc_addr==NULL){
		DMSG((STDOUT,"wrong address, failed \r\n"));
		return;
	}

	
	// check if the key-mapdata in
	data = mem_list_finddata( module, alloc_addr );
	if (NULL != data){
		// find the key-mapdata, delete to release memory		
		// delete from head list
		mem_list_deldata( module, data );
		
		// add to free head list
		mem_list_freedata( module, data );
	}
}

//	reset module state
void		
mem_resetmodule( mem_module_t *module )
{
	e_uint8 *mem;
	e_uint32 size;

	if(!mem_modulestate(module))
		return;

	mem = module->mem;
	size = module->size;

	BZERO(module, mem_module_t );
	INIT_LIST_HEAD( &module->used_list_head );
	INIT_LIST_HEAD( &module->free_list_head );
	
	module->mem = mem;
	module->size = size;
	mem_init_pool(module);
}

//	output
void		
mem_outputmodule( mem_module_t *module, char *filename )
{
	file_t file;
	e_int8 buffer[1024];
	mem_node_t *data;
	struct list_head *node,*head;
	if(!mem_modulestate(module))
		return;
	
	fi_open(filename,_O_CREATE|_O_READ|_O_WRITE,&file);
	sprintf(buffer,"file:\tline:\talloc:\tsize:\t\n");
	fi_write(buffer,1,strlen(buffer),&file);

	node = module->used_list_head.next;
	head = &module->used_list_head;

	DMSG((STDOUT,"begin to print mem-leak msg:......\r\n"));
	while(node!=head){
		data = list_entry(node,mem_node_t,list_node);
		sprintf(buffer,"%s\tline:%d\taddr:0x%x\tsize:%d\n",data->file,data->line,data->alloc_addr,data->size);
		DMSG((buffer));
		fi_write(buffer,1,strlen(buffer),&file);
		node = node->next;
	}

	DMSG((STDOUT,"end to print mem-leak msg:......\r\n"));
	fi_close(&file);
}

//	mem-leak start and exit
void		
mem_start( )
{
	mem_createmodule( &mem_leak_module );
}

void	
mem_exit( char *filename )
{
	mem_outputmodule( &mem_leak_module, filename );
	mem_closemodule( &mem_leak_module );
}
//	mem-leak malloc and free
void*		
mem_malloc( e_int32 size, char *file, e_uint32 line )
{
	void *p = malloc(size);
	if(!mem_modulestate(&mem_leak_module)){
		mem_createmodule(&mem_leak_module);
	}
	mem_appendnode( &mem_leak_module,0,(e_uint32)p,size,file,line);
	return p;
}

void		
mem_free( void *memblock )
{
	free(memblock);
	mem_deletenode( &mem_leak_module,(e_uint32)memblock);
}

void
mem_memset( void *dst, int val, int count )
{
	memset( dst, val, count );
}

//	mem-leak reset and output
void		
mem_reset( )
{
	mem_resetmodule( &mem_leak_module );
}

void		
mem_output( char *filename )
{
	mem_outputmodule( &mem_leak_module, filename );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#else
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void*		
mem_malloc( e_int32 size )
{
	if( size <=0 ) return NULL;
	return malloc( size );
}

void		
mem_free( void *memblock )
{
	if( memblock==NULL )
		return;
	free( memblock );
}

void
mem_memset( void *dst, int val, int count )
{
	memset( dst, val, count );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif	//	MEM_LEAK_DEBUG
#endif	//	ANDROID_OS
