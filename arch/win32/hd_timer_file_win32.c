//-----------------------------------------------------------------------------
//	file:		hd_timer_file_win32.c
//	author:		Joy.you
//	date:		2013-05-08
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//	MS_WIN32
#ifdef MS_WIN32

#define e_atol atol
//------------------------------------------------------------------------------

#include <hd_plat_base.h>
#include <hd_timer_api.h>

//	MS_WIN32 system function
#include <windows.h>

/*
*	GetSysTime
*	get system time
*/
void
GetSysTime( system_time_t *sys_time )
{
	SYSTEMTIME in_sys_time;
	GetSystemTime( &in_sys_time );
	memcpy( sys_time, &in_sys_time, ALIGN(sizeof(system_time_t)));
}

/*
*	GetLocalTime
*	get system local time
*/
void
GetLocalTime( system_time_t *sys_time )
{
	SYSTEMTIME in_sys_time;
	GetLocalTime( &in_sys_time );
	memcpy( sys_time, &in_sys_time, ALIGN(sizeof(system_time_t)));
}


/*
*	GetTickCount
*/

uint32_t
GetTickCount( )
{
	return GetTickCount();
}

/*
*	Delay
*/

void 
Delay( int32 delay_val )
{
	Sleep( delay_val );
}

/*
*	FileTime2LocalTime( e_uint32 value, time_tm_t *tm );
*/

void 
FileTime2LocalTime( const file_time_t *ftime, system_time_t *time )
{
	FILETIME FileTime = {0};
	SYSTEMTIME SysTime = {0};

	memcpy( &FileTime, ftime, sizeof(file_time_t));
	FileTimeToSystemTime( &FileTime,&SysTime );
	memcpy( time, &SysTime, sizeof(system_time_t) );
}

/*
*	Date2Systime( const char *date, system_time_t *time );
*/
int
Date2Systime( const char *date, system_time_t *time )
{
	int i=0, num;
	char temp[32] = {0};
	//	format: 4 2 2--2 2 2, len = 14
	//	example: 20110403102345
	if( date == NULL || time == NULL )
		return 0;

	BZERO( time, system_time_t );
	num = strlen( date );
	if( num<14 )
		return 0;


	//	copy year
	memcpy( temp,date+i , 4 );
	time->year = e_atol( temp );
	memset( temp, '\0', sizeof(temp) );
	i+= 4;

	//	copy month
		memcpy( temp,date+i , 2 );
	time->month = e_atol( temp );
	memset( temp, '\0', sizeof(temp) );
	i+= 2;

	//	copy day
	memcpy( temp,date+i , 2 );
	time->day = e_atol( temp );
	memset( temp, '\0', sizeof(temp) );
	i+= 2;

	//	copy hour
	memcpy( temp,date+i , 2 );
	time->hour = e_atol( temp );
	memset( temp, '\0', sizeof(temp) );
	i+= 2;

	//	copy minute
	memcpy( temp,date+i , 2 );
	time->minute = e_atol( temp );
	memset( temp, '\0', sizeof(temp) );
	i+= 2;

	//	copy second
	memcpy( temp,date+i , 2 );
	time->second = e_atol( temp );
	memset( temp, '\0', sizeof(temp) );
	return 1;
}


/*
*	Systime2Date( const system_time_t *time, char *date );
*/
void
Systime2Date( const system_time_t *time, char *date )
{
	int year, month, day, hour, minute, second;
	char sday[8]={0}, smonth[8]={0}, syear[8]={0};
	char shour[8]={0}, sminute[8]={0}, ssecond[8]={0};

	year	= time->year;
	month	= time->month;
	day		= time->day;
	hour	= time->hour;
	minute	= time->minute;
	second	= time->second;

	(day<10)?(sday[0]=0,sday[1]=day):(sprintf(sday,"%d",day));
	(month<10)?(smonth[0]=0,smonth[1]=day):(sprintf(smonth,"%d",month));
	(year<10)?(syear[0]=0,syear[1]=day):(sprintf(syear,"%d",year));

	(hour<10)?(shour[0]=0,shour[1]=hour):(sprintf(shour,"%d",hour));
	(minute<10)?(sminute[0]=0,sminute[1]=minute):(sprintf(sminute,"%d",minute));
	(second<10)?(ssecond[0]=0,ssecond[1]=second):(sprintf(ssecond,"%d",second));

	sprintf( date, "%s%s%s%s%s%s",syear, smonth, sday,hour, minute, second );
}

//	EOF: emap_timer_file_556.c

#endif //MS_WIN32
