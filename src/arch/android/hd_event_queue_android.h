/* hd event loop file
 * the event loop used on android platform to perform as a system level event
 * queue which recieves input events frome android framework.
 *
 * author: Joy.you
 * email: yjcpui@gmail.com
 * date: 2013-05-08
 * inspired by SDL event loop module
 */
#ifndef _EVENT_QUEUE_ANDROID_H_
#define _EVENT_QUEUE_ANDROID_H_

#ifdef ANDROID_OS

#include <arch/hd_event_api.h>

typedef enum
{
	ADDEVENT,
	PEEPEVENT,
	GETEVENT
}EventAction;

#ifdef __cplusplus
extern "C"
{
#endif
e_uint32 event_queue_init(e_uint32 flags);
void event_queue_stop(void);
e_uint32 event_queue_status(void);

e_uint32 event_queue_lock(void);
e_uint32 event_queue_unlock(void);

e_uint32 event_queue_has_event(void);
e_uint32 event_queue_push_event(Event* event);
e_uint32 event_queue_pump_event(Event* event);
e_uint32 event_queue_wait_event(Event* event);
e_uint32 event_queue_peep_event(Event* event);
#ifdef __cplusplus
}
#endif

#endif	// ANDROID_OS
#endif	// _EVENT_QUEUE_ANDROID_H_
