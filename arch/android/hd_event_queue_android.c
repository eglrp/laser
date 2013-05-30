/* Android Event Queue
 * Implemented through static cyclic queue
 *
 * author: Joy.you
 * email: yjcpui@gmail.com
 * date: 2013-05-08
 */
#ifdef ANDROID_OS

#include <hd_event_api.h>
#include <hd_thread_api.h>
#include <hd_timer_api.h>
#include "hd_event_queue_android.h"

#include <sched.h>

#ifdef DMSG
#undef DMSG
#define DMSG
#endif

#define MAXEVENTS 128

/* Event Queue Structure
 */
static struct
{
	mutex_t lock; // queue lock
	int active;   // queue active flag
	int head;     // queue head
	int tail;     // queue tail
	Event event[MAXEVENTS];
}EventQueue;

/* Event Lock
 * Event lock structure for a single event
 */
static struct
{
	mutex_t lock;
	int safe;
}EventLock;

static e_uint32 event_queue_add_event(Event* event)
{
	int tail, added;

	tail = (EventQueue.tail +1) % MAXEVENTS;
	if(tail == EventQueue.head){
		added = 0;
	}else{
		//DMSG((STDOUT,"EventQueue.tail:%d\n", tail));
		EventQueue.event[EventQueue.tail] = *event;
		EventQueue.tail = tail;
		added = 1;
	}
	return added;
}

static e_uint32 event_queue_cut_event(int spot)
{
	if(spot == EventQueue.head){
		EventQueue.head = (EventQueue.head +1) % MAXEVENTS;
		return EventQueue.head;
	}else
		if((spot + 1) % MAXEVENTS == EventQueue.tail){
			EventQueue.tail = spot;
			return EventQueue.tail;
		}else
		{
			int here, next;
			if(--EventQueue.tail < 0){
				EventQueue.tail = MAXEVENTS - 1;
			}
			for(here = spot; here != EventQueue.tail; here = next){
				next = (here + 1) % MAXEVENTS;
				EventQueue.event[here] = EventQueue.event[next];
			}
			return spot;
		}
}

/* lock the event queue, take a peep at it, then unlock it
 */

static e_uint32 event_queue_peep_events(Event* events, int numevents, EventAction action)
{
	int i, used;
	if(!EventQueue.active){
		return -1;
	}

	used = 0;
	if(mutex_lock(&EventQueue.lock) == 1){
		//DMSG((STDOUT,"EventQ locked \n"));

		if(action == ADDEVENT)
		{
			for(i = 0; i < numevents; i++){
				DMSG((STDOUT,"add event\n"));
				used += event_queue_add_event(&events[i]);
			}
		}
		else
		{
			Event tmpevent;
			int spot;
			if(events == NULL){
				action = PEEPEVENT;
				numevents = 1;
				events = &tmpevent;
			}
			spot = EventQueue.head;
			while((used < numevents) && (spot != EventQueue.tail)){
				events[used++] = EventQueue.event[spot];
				if(action == GETEVENT){
					//DMSG((STDOUT,"get Event: type %s\n",(char *)event_to_str(EventQueue.event[spot].type)));
					spot = event_queue_cut_event(spot);
				}else{
					spot = (spot + 1) % MAXEVENTS;
				}
			}
		}
		mutex_unlock(&EventQueue.lock);
	}else{
		DMSG((STDOUT,"Couldn't lock the event queue\n"));
		used = -1;
	}
	return used;
}

e_uint32 event_queue_lock(void)
{
	if(EventLock.safe){
		mutex_lock(&EventLock.lock);
		EventQueue.active = 0;
		mutex_unlock(&EventLock.lock);
		return 0;
	}
	else
		return -1;
}

e_uint32 event_queue_unlock(void)
{
	if(EventLock.safe){
		mutex_lock(&EventLock.lock);
		EventQueue.active = 1;
		mutex_unlock(&EventLock.lock);
		return 0;
	}
	else
		return -1;
}

void event_queue_stop(void)
{
	/* clear ems event queue
	 */
	EventQueue.active = 0;
	EventQueue.head = 0;
	EventQueue.tail = 0;
	EventLock.safe = 0;

	DMSG((STDOUT,"destroying event mutexes"));
	mutex_destroy(&(EventLock.lock));
	mutex_destroy(&(EventQueue.lock));
}

e_uint32 event_queue_init(e_uint32 flags)
{
	mutex_init(&(EventQueue.lock));
	mutex_init(&(EventLock.lock));

	EventQueue.active = 1;
	EventLock.safe = 1;

	return 0;
}

e_uint32 event_queue_status(void)
{
	mutex_lock(&EventLock.lock);
	e_uint32 ret = EventQueue.active;
	mutex_unlock(&EventLock.lock);

	return !ret?0:1;
}

e_uint32 event_queue_has_event(void)
{
	return (event_queue_peep_events(NULL, 0, PEEPEVENT) > 0);
}

e_uint32 event_queue_push_event(Event* event)
{
	DMSG((STDOUT,"EventQPushEvent: push event\n"));
	if(event_queue_peep_events(event, 1, ADDEVENT) <= 0){
		return -1;
	}
	return 1;
}

e_uint32 event_queue_pump_event(Event* event)
{
	DMSG((STDOUT,"calling EventQPumpEvent\n"));
	if(event_queue_peep_events(event, 1, GETEVENT) <= 0){
		DMSG((STDOUT,"Event Fetch Failded\n"));
		return 0;
	}
	else{
		DMSG((STDOUT,"Event Fetched: type %d\n", event->type));
		return 1;
	}
}

e_uint32 event_queue_wait_event(Event* event)
{
	DMSG(("calling event_queue_wait_event\n"));
	while(1){
		if(event_queue_peep_events(event, 1, GETEVENT) <= 0){
			//Delay(1);
			sched_yield();
		}
		else{
			DMSG((STDOUT,"event type %d\n", event->type));
			return 1;
		}
	}
}

e_uint32 event_queue_peep_event(Event* event)
{
	if(event_queue_peep_events(event, 1, PEEPEVENT) <= 0){
		return -1;
	}
	DMSG((STDOUT,"EventPeeped: type %d\n", event->type));
	return 1;
}

#endif
