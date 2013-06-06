//-----------------------------------------------------------------------------
//	file:		hd_timer_api.h
//
//	author: Joy.you
//	2013-05-08
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#ifndef _HD_TIMER_API_H_
#define _HD_TIMER_API_H_

#include "hd_plat_base.h"

//------------------------------------------------------------------------------
// System time is represented with the following structure:
//
typedef struct{
	e_uint16 year;
	e_uint16 month;
	e_uint16 dayofweek;
	e_uint16 day;
	e_uint16 hour;
	e_uint16 minute;
	e_uint16 second;
	e_uint16 mmseconds;
}system_time_t;

typedef struct{
	e_uint32 data_time_l;
	e_uint32 data_time_h;
}file_time_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//	void				GetSysTime( system_time_t *sys_time )
//	e_uint32	GetTickCount();
//	void				Delay( e_int32 delay_val );
//
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//------------------------------------------------------------------------------
//

/*
*	GetSysTime
*	get system time
*/
void DEV_EXPORT 
GetSysTime( system_time_t *sys_time );

/*
*	GetLocalTime
*	get system local time
*/
void DEV_EXPORT 
GetLocalTime( system_time_t *sys_time );


/*
*	GetTickCount,
*	in usec,us
*/

e_uint32 DEV_EXPORT 
GetTickCount( );

/*
*	Delay
*/

void DEV_EXPORT 
Delay( e_int32 delay_val );


/*
*	FileTime2LocalTime( const file_time_t *ftime, system_time_t *time );
*/

void DEV_EXPORT
FileTime2LocalTime( const file_time_t *ftime, system_time_t *time );

//------------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
//
#endif	//	
//	EOF: hd_timer_api.h
