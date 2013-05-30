//-----------------------------------------------------------------------------
//	hd_event_api.h
//	Joy.you
//	2013-05-08
//
//
//
//------------------------------------------------------------------------------

#ifndef _EVENT_API_H_
#define _EVENT_API_H_

#include <hd_plat_base.h>
#include <hd_thread_api.h>

//------------------------------------------------------------------------------

/* key and mouse state */
#define RELEASE		0
#define PRESSED		1

/* base mouse id  */
#define BUTTON_LEFT			0
#define BUTTON_RIGTH		1
#define BUTTON_MIDDLE		2

/* peek message flag */
#define PM_REMOVE			0
#define PM_NOREMOVE			1

/* Event enumerations */
typedef enum {
	NOEVENT = 0,	/* Unused (do not remove) */
	ACTIVEEVENT,			/* Application gains visibility */
	INACTIVEEVENT,		/* Application loses visibility */
	KEYDOWN,				/* Keys pressed */
	CHAR,					/* key values */
	KEYUP,				/* Keys released */
	MOUSEMOTION,			/* Mouse moved */
	MOUSEBUTTONDOWN,		/* Mouse button pressed */
	MOUSEBUTTONUP,		/* Mouse button released */
    MOUSE2MOTION,			/* Two Mouse moved */
	MOUSE2BUTTONDOWN,		/* Two Mouse button pressed */
	MOUSE2BUTTONUP,		/* Two Mouse button released */
	VIDEORESIZE,			/* User resized video mode */
	VIDEOEXPOSE,			/* Screen needs to be redrawn */
	TIMER,				/* Reserved for future use.. */
	USBIN,		/* usb message in */
	USBOUT,		/* usb message out */
	SDCARDIN,		/* sdcard message in */
	SDCARDOUT,		/* sdcard message out */
	/* Events USEREVENT through MAXEVENTS-1 are for your use */
	USEREVENT,
	COMMEVENT,
	COMMRESET,
	/* This last event is only for bounding internal arrays
	It is the number of bits in the event mask datatype -- e_uint32
    */
	QUIT,				/* User-requested quit */
	NUMEVENTS
} EventType;

/* Application visibility event structure */
typedef struct ActiveEvent {
	e_uint32 type;	/* ACTIVEEVENT */
	e_uint32 gain;	/* Whether given states were gained or lost (1/0) */
	e_uint32 state;	/* A mask of the focus states */
	e_uint32 unused;
} ActiveEvent;

/* Keyboard event structure */
typedef struct KeyboardEvent {
	e_uint32 type;	/* KEYDOWN or KEYUP */
	e_uint32 which;	/* The keyboard device index */
	e_uint32 state;	/* PRESSED or RELEASED */
	e_uint32 key;
} KeyboardEvent;

/* Mouse motion event structure */
typedef struct MouseMotionEvent {
	e_uint32 type;	/* MOUSEMOTION */
	e_uint32 which;	/* The mouse device index */
	e_uint32 state;	/* The current button state */
	e_uint32 unused1;

	e_int16 x, y;	/* The X/Y coordinates of the mouse */
	e_int16 xrel;	/* The relative motion in the X direction */
	e_int16 yrel;	/* The relative motion in the Y direction */
} MouseMotionEvent;

/* Mouse button event structure */
typedef struct MouseButtonEvent {
	e_uint32 type;	/* MOUSEBUTTONDOWN or MOUSEBUTTONUP */
	e_uint32 which;	/* The mouse device index */
	e_uint32 button;	/* The mouse button index */
	e_uint32 state;	/* PRESSED or RELEASED */
	e_int16 x, y;	/* The X/Y coordinates of the mouse at press time */
} MouseButtonEvent;

/* Mouse two button event structure */
typedef struct Mouse2TouchEvent {
	e_uint32 type;	/* MOUSE2BUTTONDOWN or MOUSE2BUTTONUP or MOUSE2MOTION */
	e_uint32 which;	/* The mouse device index */
	e_uint32 state;	/* PRESSED or RELEASED */
    
    e_int16 x0, y0;	/* The X/Y coordinates of the first mouse */
    e_int16 x1, y1;	/* The X/Y coordinates of the second mouse */
} Mouse2TouchEvent;

/* The "window resized" event
   When you get this event, you are responsible for setting a new video
   mode with the new width and height.
 */
typedef struct ResizeEvent {
	e_uint32 type;	/* VIDEORESIZE */
	e_uint32 w;		/* New width */
	e_uint32 h;		/* New height */
} ResizeEvent;

/* The "screen redraw" event */
typedef struct ExposeEvent {
	e_uint32 type;	/* VIDEOEXPOSE */
} ExposeEvent;

/* The "quit requested" event */
typedef struct QuitEvent {
	e_uint32 type;	/* QUIT */
} QuitEvent;

/* A user-defined event type */
typedef struct UserEvent {
	e_uint32 type;	/* USEREVENT through NUMEVENTS-1 */
	e_uint32 code;	/* User defined event code */
	e_uint32 wparam;	/* User defined data pointer */
	e_uint32 lparam;	/* User defined data pointer */
} UserEvent;

/* system timer message */
typedef struct TimerEvent{
	e_uint32 type;			//	TIMER
	e_uint32 timer_id;
	e_uint32 dwtime;
}TimerEvent;

/* General event structure */
typedef union Event {	
	e_uint32 type;
	
	ActiveEvent active;
	KeyboardEvent key;
	MouseMotionEvent motion;
	MouseButtonEvent button;
    Mouse2TouchEvent twotouch;
	ResizeEvent resize;
	ExposeEvent expose;
	QuitEvent quit;
	TimerEvent timer;
	UserEvent user;

} Event;

/* EMS Key Map */
typedef enum {	
	EMSK_UNKNOWN = 0,
	EMSK_ESCAPE = 1,
	EMSK_TAB = 2,
	EMSK_MENU = 3,
	EMSK_UP = 4,
	EMSK_LEFT = 5,
	EMSK_RIGHT = 6,
	EMSK_DOWN = 7,
	EMSK_F1 = 8,
	EMSK_F2 = 9,
	EMSK_RETURN = 10,/*EMSKEY_ENTER*/	
	EMSK_F4 = 11,
	EMSK_F3 = 12,	
	EMSK_UNKNOWN1 = 13,/*EMSKEY_ESC*/
	EMSK_UNKNOWN2 = 14,/*{EMSKEY_START,*/
	EMSK_LAST
} Key;

//------------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//------------------------------------------------------------------------------
//

//	get system message
e_uint32 DEV_EXPORT
PumpEvents( Event *event );

e_uint32 DEV_EXPORT
PeekEvents( Event *event, e_uint32 remove_flag );

void  DEV_EXPORT
PostEvents( e_uint32 msg, e_uint32 wparam, e_uint32 lparam );

void DEV_EXPORT
SendEvents( e_uint32 msg, e_uint32 wparam, e_uint32 lparam );
//------------------------------------------------------------------------------
//
e_uint8 DEV_EXPORT
PostThreadEvent( ethread_t *thread, e_uint32 msg, e_uint32 wparam, e_uint32 lparam );

e_uint32 DEV_EXPORT
QueuePump( void *queue, Event *event );

e_uint32 DEV_EXPORT
QueuePeek( void *queue, Event *event, e_uint32 remove );

e_uint32 DEV_EXPORT
QueuePost( void *queue, e_uint32 msg, e_uint32 wparam, e_uint32 lparam );

void DEV_EXPORT
SetEventData( e_uint32 wparam, e_uint32 lparam, Event *event );

//------------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------
//

//------------------------------------------------------------------------------
//
#endif	//	_SCREEN_BASE_H_
// EOF: hd_screen_base.h
