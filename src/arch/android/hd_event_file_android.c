/*
 * file: hd_event_file_android.cpp
 * date: 2013-05-08
 * author: Joy.you
 */

#include <arch/hd_event_api.h>

#ifdef ANDROID_OS

#include <arch/hd_thread_api.h>

#include "../internal/hd_inter_event.h"
#include "hd_event_queue_android.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


int g_mouse_state;

void
SetEventData(e_uint32 wparam, e_uint32 lparam, Event *event)
{
	event_data_t *eventdata;

	if(lparam ==0x0)
		return;
	//	1 lock eventman
	//	2 set data and free node
	//	3 unlock eventman
	eventdata = (event_data_t*)lparam;
	event->type = eventdata->usr_msg;
	event->user.wparam = eventdata->wparam;
	event->user.lparam = eventdata->lparam;

	internal_eventman_free(eventdata);
}

#ifdef DMSG
char* event_str[] =
{
	"NOEVENT", /* Unused (do not remove) */
	"ACTIVEEVENT", /* Application gains visibility */
	"INACTIVEEVENT", /* Application loses visibility */
	"KEYDOWN", /* Keys pressed */
	"CHAR", /* key values */
	"KEYUP", /* Keys released */
	"MOUSEMOTION", /* Mouse moved */
	"MOUSEBUTTONDOWN", /* Mouse button pressed */
	"MOUSEBUTTONUP", /* Mouse button released */
	"VIDEORESIZE", /* User resized video mode */
	"VIDEOEXPOSE", /* Screen needs to be redrawn */
	"TIMER", /* Reserved for future use.. */
	"USBIN", /* usb message in */
	"USBOUT", /* usb message out */
	"SDCARDIN", /* sdcard message in */
	"SDCARDOUT", /* sdcard message out */
	"USEREVENT",
	"COMMEVENT",
	"COMMRESET",
	"QUIT", /* User-requested quit */
	"UNKNOWN",
	"NUMEVENTS"
};

char *event_to_str(EventType type)
{
	if(type >= sizeof(event_str)/sizeof(char*))
		type = sizeof(event_str)/sizeof(char*)-2;
	return event_str[type];
}
#endif

/** Main thread(gui thread) event operations
 * pump, peek, post, and send events to/from main thread event queue
 */
e_uint32
PumpEvents(Event *event)
{
	// if(!SystemState())
	// 	return 0;
	if(!event_queue_status())
		return 0;
	event_queue_wait_event(event);
	//DMSG((STDOUT,"event pumped: type:%d, name:%s\n", event->type, event_to_str(event->type)));
	return 1;
}

e_uint32
PeekEvents(Event *event, e_uint32 remove_flag)
{
	// if(!SystemState())
	// 	return 0;
	if(!event_queue_status())
		return 0;

	switch(remove_flag)
	{
	case PM_REMOVE:
		event_queue_pump_event(event);
		break;
	case PM_NOREMOVE:
		event_queue_peep_event(event);
		break;
	default:
		break;
	}
	return 1;
}

void
PostEvents(e_uint32 msg, e_uint32 wparam, e_uint32 lparam)
{
	if(!event_queue_status())
		return;

	Event event =
		{	0};

	event.type = msg;
	event.user.wparam = wparam;
	event.user.lparam = lparam;

	event_queue_push_event(&event);

	DMSG((STDOUT,"posting event to queue, type:%d, name:%s", (int)event.type,
			event_to_str(event.type)));

	return;
}

void
SendEvents(e_uint32 msg, e_uint32 wparam, e_uint32 lparam)
{
	PostEvents(msg, wparam, lparam);
}

/** Event operations for other threads
 */
e_uint8
PostThreadEvent(ethread_t *thread, e_uint32 msg,
		    e_uint32 wparam, e_uint32 lparam)
{
	if(!thread || !threadstate(thread))
	{
		DMSG((STDOUT,"PostThreadEvent: thread->queue==0x0"));
		return 0;
	}
	return QueuePost(thread->queue, msg, wparam, lparam);
}

e_uint32
QueuePump(void *queue, Event *event)
{
	e_uint32 rsize = 0;
	event_data_t *event_data = NULL;

	if(!queue)
		return 0;

	DMSG((STDOUT,"pumping event from thread\n"));
	rsize = read(((int*)(queue))[0],
		     (void*)&event_data,
		     sizeof(event_data));

	DMSG((STDOUT,"thread event pumped:%p, size:%d", event_data, (unsigned int)rsize));
	if(rsize != sizeof(event_data))
	{
		return 0;
	}

	DMSG((STDOUT,"set event data from event pool\n"));
	SetEventData(0, (e_uint32)event_data, event);

	return 1;
}

e_uint32
QueuePeek(void *queue, Event *event, e_uint32 remove)
{
	if(!queue)
		return 0;
	return 1;
}

e_uint32
QueuePost(void *queue, e_uint32 msg, e_uint32 wparam, e_uint32 lparam)
{
	if(!queue)
		return 0;

	e_uint32 wsize = 0;
	event_data_t *event;

	DMSG((STDOUT,"posting event to thread queue"));
	/* 1 cal user event position, and make message data */
	event = internal_eventman_alloc();
	if(event== 0x0)
	{
		DMSG((STDOUT,"event man does not have enough memory, post failed\n"));
		return 0;
	}
	DMSG((STDOUT,"internal event pool allocated"));

	/* 2 set data for user event table */
	event->usr_msg = msg;
	event->wparam = wparam;
	event->lparam = lparam;

	/* 3 send event pointer to thread pipe */
	DMSG((STDOUT,"posting thread event to pipe"));
	wsize = write(((int*)(queue))[1], &event, sizeof(event));

	if(wsize != sizeof(event))
	{
		return 0;
	}
	DMSG((STDOUT,"thread event posted"));
	return 1;
}

/* extern e_uint32 g_key_state,g_mouse_state; */
e_uint32 g_key_state,g_mousstate;

e_uint32 push_event_with_params(EventType type, ...)
{
	if(!event_queue_status())
		return 0;

	Event event = {0};

	va_list arg = {0};

	va_start(arg, type);
	event.type = type;

	DMSG((STDOUT,"sending event: type:%s \n",event_to_str(type)));
	switch(type)
	{
	case MOUSEMOTION:
	{
		g_mouse_state = event.motion.state = PRESSED;
		event.motion.x = va_arg(arg, int);
		event.motion.y = va_arg(arg, int);
		event.motion.xrel = va_arg(arg, int);
		event.motion.yrel = va_arg(arg, int);
		break;
	}
	case MOUSEBUTTONDOWN:
	case MOUSEBUTTONUP:
	{
		g_mouse_state = event.button.state = type == MOUSEBUTTONDOWN ? PRESSED : RELEASE;
		event.button.x = va_arg(arg, int);
		event.button.y = va_arg(arg, int);
		break;
	}
	case TIMER:
	{
		event.timer.timer_id = va_arg(arg, int);
		DMSG((STDOUT,"TIMER id:%d", (unsigned int)event.timer.timer_id));
		break;
	}
	case CHAR:
	{
		g_key_state = event.key.state = RELEASE;
		event.key.key = va_arg(arg, int);
		break;
	}
	case KEYDOWN:
	case KEYUP:
	{
		g_key_state = event.key.state = type == KEYDOWN ? PRESSED : RELEASE;
		event.key.key = va_arg(arg, int);
		break;
	}
	default:
		return 0;
		break;
	}

	va_end(arg);
	return event_queue_push_event(&event);
}

#endif /*ANDROID_OS*/
