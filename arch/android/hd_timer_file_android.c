/** file: hd_timer_file_android.c
 * author: Joy.you
 * email: yjcpui@gmail.com
 * date: 2013-05-08
 * ANDROID_OS
 */
#ifdef ANDROID_OS

#include <hd_plat_base.h>
#include <hd_timer_api.h>

#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/** time structure convert util
 * util func to convert posix tm structure to ems time structure
 * there may be bugs before testing
 */

static void posixtm2emstm(struct tm *posix_tm, system_time_t *tm) {
	tm->year = (e_uint16) (posix_tm->tm_year + 1900);
	tm->month = (e_uint16) (posix_tm->tm_mon);
	tm->dayofweek = (e_uint16) (posix_tm->tm_wday);
	tm->day = (e_uint16) (posix_tm->tm_mday);
	tm->hour = (e_uint16) (posix_tm->tm_hour);
	tm->minute = (e_uint16) (posix_tm->tm_min);
	tm->second = (e_uint16) (posix_tm->tm_sec);
	tm->mmseconds = 0;
}

/** GetSysTime
 * get system time
 */
void GetSysTime(system_time_t *sys_time) {
	struct tm *sys_time_tm;
	time_t sys_time_sec;
	time(&sys_time_sec);
	sys_time_tm = gmtime(&sys_time_sec);
	if (sys_time_tm == 0x0)
		return;

	posixtm2emstm(sys_time_tm, sys_time);
}

/** GetLocalTime
 * get system local time
 */
void GetLocalTime(system_time_t *sys_time) {
	struct tm *sys_time_tm;
	time_t sys_time_sec;
	time(&sys_time_sec);
	sys_time_tm = localtime(&sys_time_sec);
	if (sys_time_tm == 0x0)
		return;

	posixtm2emstm(sys_time_tm, sys_time);
}

/** GetTickCount
 * in us
 */
e_uint32 GetTickCount() {
	struct timeval tv;
	if (gettimeofday(&tv,NULL)!=0x0)
		return 0;

	return (tv.tv_sec * 1000000) + (tv.tv_usec);
//	return (e_uint32)clock() * 1000000/CLOCKS_PER_SEC;
}

/** Delay
 * sleep for delay_val ms
 */
void Delay(e_int32 ms) {
	int was_error;
	struct timespec elapsed, tv;
	elapsed.tv_sec = ms / 1000;
	elapsed.tv_nsec = (ms % 1000) * 1000000;

	/* sleep tv time despite of interrupting signal */
	do {
		errno = 0;
		tv.tv_sec = elapsed.tv_sec;
		tv.tv_nsec = elapsed.tv_nsec;
		was_error = nanosleep(&tv, &elapsed);
	} while (was_error && (errno == EINTR));
}

#endif	/* ANDROID_OS */
