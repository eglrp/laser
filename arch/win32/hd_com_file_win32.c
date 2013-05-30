//----------------------------------------------------------------------------
//	hd_com_file_win32.c
//  kevin.ban
// 	2008-01-20
//
//
//
//----------------------------------------------------------------------------
#ifdef MS_WIN32
#include <hd_com_api.h>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
///	MS_WIN32 system function
#include <windows.h>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define COM_MAX_INBUFSIZE	10000
#define COM_MAX_OUTBUFSIZE	10000

#define COM_BUFFER_ITEM_SIZE	128
#define COM_BUFFER_MAX_NUM		128
#define COM_BUFFER_MEM_SIZE 	(COM_BUFFER_ITEM_SIZE*(COM_BUFFER_MAX_NUM+1))

//	total memory size is: 128*128/1024=16KB
static uint8_t		com_buffer_mem[COM_BUFFER_MEM_SIZE];
static struct list_head	com_pool_head;
static uint32_t		com_pool_size;

//----------------------------------------------------------------------------
typedef struct{
	struct list_head list_node;
	char			 data[COM_BUFFER_ITEM_SIZE];
}com_pool_data_t;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void
comm_init_pool( uint8_t *mem, uint32_t mem_size, struct list_head *free_list_head )
{
	uint8_t *com_mem;
	uint32_t avail_mem_size, data_mem_size;
	// set
	com_mem				= mem;
	avail_mem_size		= mem_size;
	data_mem_size		= ALIGN(sizeof(com_pool_data_t));
	com_pool_size		= 0;
	
	INIT_LIST_HEAD(free_list_head);

	// dispose
	while (avail_mem_size >= data_mem_size){
		com_pool_data_t *data = (com_pool_data_t *)com_mem;
		
		// add to list		
		list_add_before(&data->list_node, free_list_head);
        
		com_mem		    += data_mem_size;
		avail_mem_size	-= data_mem_size;
		com_pool_size++;
	}
}

static com_pool_data_t *
comm_alloc_data( struct list_head *free_list_head )
{
	com_pool_data_t *data;
	//	1 get next storage data nd add before head
	data = list_entry(free_list_head->next,com_pool_data_t,list_node);
	list_del(free_list_head->next);
	list_add_before(&data->list_node,free_list_head);
	return data;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
uint8_t		
Comm_Open( com_t *com, const char* com_name )
{
	COMMTIMEOUTS CommTimeouts;
	char name[32];
	BZERO( com, com_t );
	if( com_name == 0x0 ){
		return FALSE;
	}
	
	//	init com pool
	//comm_init_pool( com_buffer_mem, COM_BUFFER_MEM_SIZE, &com_pool_head );

	//	open gps comm device
	sprintf( name, "%s:", com_name );
	com->priv = CreateFileA( name,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL );
	if( com->priv==0x0 || com->priv == INVALID_HANDLE_VALUE ){
		DMSG((STDOUT,"failed to open comm: %s\r\n", com_name));
		return FALSE;
	}

	GetCommTimeouts(com->priv, &CommTimeouts);
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 400;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 5000;
	if (!SetCommTimeouts (com->priv, &CommTimeouts)){ 
		DMSG((STDOUT,"failed to SetCommTimeouts, err: %d\r\n", GetLastError()));
		return FALSE;
    }

	SetCommMask( com->priv, EV_RXCHAR|EV_BREAK|EV_ERR );
	com->dcb.DCBlength = sizeof( com_dcb_t );
	GetCommState( com->priv, (DCB*)&(com->dcb) );

	//sleep to let dcb be effect
	Sleep(1000);
	DMSG((STDOUT,"succeed to open comm: %s addr: 0x%X err %d\r\n", com_name, com->priv, GetLastError() ));

	com->state = TRUE;
	sprintf( com->name, "%s", com_name );
	return TRUE;
}

void				
Comm_Close( com_t *com )
{
	if( com && !Comm_State(com) )
		return;
	
	//	close gps comm device
	CloseHandle( com->priv );
	BZERO( com, com_t );
}

uint8_t 
Comm_State( com_t *com )
{
	if( com == 0x0 )
		return 0;
		
	return ((com->state==TRUE)?TRUE:FALSE);
}

//	attach and detach thread object
ethread_t *
Comm_Attach( com_t *com, ethread_t *thread )
{
	ethread_t *old;	
	old = com->thread;
	com->thread = thread;
	return old;
}

void			
Comm_Detach( com_t *com )
{
	if( !Comm_State(com) )
		return;

	com->thread = 0x0;
}

//	get/set comm state
bool 		
Comm_GetData( com_t *com, com_dcb_t *dcb )
{
	if( !Comm_State(com) || !dcb )
		return FALSE;
		
	dcb->DCBlength = sizeof (DCB);
	if( !GetCommState( com->priv, (DCB*)dcb) )
		return FALSE;

	dcb->ByteSize = 8;
	dcb->Parity = NOPARITY;
	dcb->StopBits = ONESTOPBIT;		
	com->dcb = *dcb;
	return TRUE;
}

bool 	
Comm_SetData( com_t *com, const com_dcb_t *dcb )
{
	if( !Comm_State(com)|| !dcb )
		return FALSE;
			
	if( !SetCommState( com->priv, (DCB*)dcb )||
		!SetupComm( com->priv, COM_MAX_INBUFSIZE, COM_MAX_OUTBUFSIZE ))
		return FALSE;

	com->dcb = *dcb;
	return TRUE;
}

//	get/set comm monitored mask
uint32_t 	
Comm_SetMask( com_t *com, uint32_t mask )
{
	uint32_t old, in_mask;
	if( !Comm_State(com) )
		return FALSE;
	
	in_mask = 0;
	if( mask&COM_EV_BREAK){
		in_mask |= EV_BREAK;
	}
	if( mask&COM_EV_CTS){
		in_mask |= EV_CTS;
	}
	if( mask&COM_EV_DSR){
		in_mask |= EV_DSR;
	}
	if( mask&COM_EV_ERR){
		in_mask |= EV_ERR;
	}
	if( mask&COM_EV_RING){
		in_mask |= EV_RING;
	}
	if( mask&COM_EV_RLSD){
		in_mask |= EV_RLSD;
	}
	if( mask&COM_EV_RXCHAR){
		in_mask |= EV_RXCHAR;
	}
	if( mask&COM_EV_RXFLAG){
		in_mask |= EV_RXFLAG;
	}
	if( mask&COM_EV_TXEMPTY){
		in_mask |= EV_TXEMPTY;
	}	
	
	if( in_mask==0 ){
		return com->mask;
	}
		
	SetCommMask( com->priv, in_mask );
	old = com->mask;
	com->mask = mask;
	return old;
}

uint32_t
Comm_GetMask( com_t *com )
{
	if( !Comm_State(com) )
		return FALSE;

	return com->mask;
}

//	clear comm output/input
bool		
Comm_Purge( com_t *com, uint32_t flag )
{
	uint32_t in_flag;
	if( !Comm_State(com) )
		return FALSE;
	
	in_flag = 0;
	if( flag&COM_PURGE_TXABORT ){
		in_flag |= PURGE_TXABORT;
	}
	if( flag&COM_PURGE_RXABORT ){
		in_flag |= PURGE_RXABORT;
	}
	if( flag&COM_PURGE_TXCLEAR ){
		in_flag |= PURGE_TXCLEAR;
	}
	if( flag&COM_PURGE_RXCLEAR ){
		in_flag |= PURGE_RXCLEAR;
	}

	if( in_flag == 0 ){
		return FALSE;
	}

	PurgeComm( com->priv, in_flag );
	return TRUE;
}

//	read comm data
uint32_t		
Comm_Read( com_t *com, uint8_t *buffer, uint32_t blen )
{
	uint32_t 	err, sum, dwBytesTransferred, event_mask;
	uint8_t 	bbyte;

	//	1 check comm state
	//	2 check system comm event
	//	3 check event type and wait again
	//	4 read message data from comm
	
	//	1 check comm state
	if( !Comm_State(com) )
		return FALSE;
	
	//	2 check system comm event
	//WaitCommEvent(com->priv,&event_mask,NULL);

	//	//3 check event type and wait again
	//if( (event_mask&EV_RXCHAR)!=EV_RXCHAR){
	//	//	here, must check system support WaitCommEvent function, some not support 
	//	err = GetLastError();
	//	switch( err ){
	//	case ERROR_INVALID_HANDLE:
	//		DMSG((STDOUT,"err: %d\tDelay 50s, mask: %d\tERROR_INVALID_HANDLE\r\n",err,event_mask));
	//		Sleep(50);
	//		return FALSE;
	//	case ERROR_CALL_NOT_IMPLEMENTED:
	//		DMSG((STDOUT,"err: %d\tDelay 10s, mask: %d\tERROR_CALL_NOT_IMPLEMENTED\r\n",err,event_mask));
	//		Sleep(10);
	//		return FALSE;
	//	}
	//}
	
	//	4 read message data from comm
	//sum = 1;
	sum = 0;
	//blen = (blen>=COM_BUFFER_ITEM_SIZE)?COM_BUFFER_ITEM_SIZE:blen;
	do{
		//	read one data
		ReadFile( com->priv, &bbyte, 1, &dwBytesTransferred, NULL);
		//	check success or not
		//if(dwBytesTransferred == 1){
		//	//	check end flag
		//	if( bbyte == '$' ){   //我们的结束符为@（0x40）
		//		buffer[sum] = '\0';
		//		break;
		//	}
		//	
		//	(sum>=blen)?(sum=1):(0);
		//	//	write data to buffer
		//	buffer[sum] = bbyte;
		//	sum++;	
		//}else{
		//	//	no data
		//	return FALSE;
		//}
		if (dwBytesTransferred == 1)
		{
				//if( bbyte == '@' ){   //我们的结束符为@（0x40），已经读到消息结尾了,不需要过滤结束符，直接给上层逻辑过滤都行
					//buffer[sum] = '\0';
					//break;
			buffer[sum] = bbyte;
			++sum;
		}
		else
		{
			DWORD dwError = GetLastError();
			return FALSE;
		}
	}while(sum < blen );

	//buffer[sum] = '\0';//以0结束，所以分配缓存大小时要多一个长度,‘\0’由上层决定
	//buffer[0] = '$';//从1开始读
	//return (sum<=10)?0:sum;
	return TRUE;
}

//	write comm data
uint32_t
Comm_Write( com_t *com, unsigned char *buffer, unsigned long len )
{
	uint32_t dwBytes;
	if (WriteFile(com->priv,buffer,len,&dwBytes,0) == 0)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
#endif //MS_WIN32

