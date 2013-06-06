//----------------------------------------------------------------------------
//	hd_dbg_print_android.c
//  Joy.you
// 	2013-05-08
//
//
//
//----------------------------------------------------------------------------
#ifdef ANDROID_OS

#include <arch/hd_dbg_print.h>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void			
debug_init( const char *name )
{
#ifndef MSG_PRINT
	printf("system does not support dbg\r\n");
#else
#endif
}

void			
debug_exit()
{
#ifndef MSG_PRINT
	printf("system does not support dbg\r\n");
#else
#endif
}

void			
debug_fullname( char *fullname )
{
#ifndef MSG_PRINT
	printf("system does not support dbg\r\n");
#else
#endif
}

#endif	//	ANDROID_OS
