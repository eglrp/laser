//-----------------------------------------------------------------------------
//	file:		hd_thread_api.h
//	author:	    Joy.you
//	date:		2013-05-08
//
//------------------------------------------------------------------------------

#ifndef	_HD_THREAD_H_
#define	_HD_THREAD_H_

//------------------------------------------------------------------------------
//

#include "hd_plat_base.h"
#include <comm/hd_list.h>

//------------------------------------------------------------------------------
//	default max thread number can be created, except main thread
#define THREAD_START	1
#define THREAD_RUN		2
#define THREAD_PAUSE	3
#define TRHEAD_RESUME	4
#define THREAD_HOLD		5
#define THREAD_EXIT		0

#define SEMA_IGNORE		0
#define SEMA_INFINITE	0xffffffff


//------------------------------------------------------------------------------
//	thread main function prototype definition
typedef e_int32 (*thread_func)(void*);

//------------------------------------------------------------------------------
//	thread data definition
typedef struct{
	void				*priv;							//
	void				*queue;							//
	void				*lock_queue;					//	thread lock queue
	e_uint32	thread_idx;						//	thread index in manager
	e_uint32 	thread_id;						//	thread id
	thread_func	func;									//	thread main function
	void 				*data;								//	thread priv data

	e_uint32	*stack_head;						//	private data, head of stack
	e_uint32	*stack_tail;						//	private data, tail of stack
	struct list_head 	list_node;						//	manager node
	
	e_uint32	state;								//	thread state, created or not	
	e_uint32	priority;
	char				name[32];
}ethread_t;

//------------------------------------------------------------------------------
//	semaphore data definition
typedef struct{
	void		*priv;			//	system private data
	e_uint32	state;			//	semaphore state

	e_int32	init_value;		//	semaphore init value
	e_int32	max_value;		//	semaphore max value, private data
}semaphore_t;

typedef struct{
	void				*priv;			//	system private data
	e_uint32	state;			//	mutex state
}mutex_t;

typedef struct{
	void		*priv;
	e_int32	state;

	semaphore_t sema;
	e_int32	waiter;
}condition_t;

//------------------------------------------------------------------------------
//
#ifdef __cplusplus
extern "C"
{
#endif
//------------------------------------------------------------------------------
//

/*	createthread
*		func: thread callback function
*		data: callback function's param
*		return value: thread flag
*/
e_int32 DEV_EXPORT
createthread( const char *name, thread_func func, void *data,
		e_uint32 priority, ethread_t **thread );

/*	killthread
*		thread:your thread
*/
void  DEV_EXPORT
killthread( ethread_t *thread );


/*
*	suspendthread:
* suspends the specified thread.
*/
e_int32 DEV_EXPORT
suspendthread( ethread_t *thread );

/*
*	resumethread:
* decrements a thread's suspend count,When the suspend count 
* is decremented to zero, the execution of the thread is resumed.
*/
e_int32 DEV_EXPORT
resumethread( ethread_t *thread );


/*
*	holdthread:
*/

e_int32 DEV_EXPORT
holdthread( ethread_t *thread );

/*	threadstate
*		get thread state
*/
e_int32 DEV_EXPORT
threadstate( ethread_t *thread );

/*	getthreadid
*		get current thread id
*/
e_int32  DEV_EXPORT
getthreadid( ethread_t *thread);

/*	thread_should_quit
*		check if thread should quit
*/
e_int32  DEV_EXPORT
threadshouldquit( ethread_t *thread);

/*	threadwait
*		wait for thread for quit
*		timeout / 1000 = seconds
*/
e_int32  DEV_EXPORT
threadwait( ethread_t *thread, e_uint32 timeout);

//------------------------------------------------------------------------------
//

/*	createsemaphore
*		create a semaphore
*/
e_uint8 DEV_EXPORT
semaphore_init(semaphore_t *semaphore, e_int32 init_value );

/*	destroysemphore
*		destroy a semaphore
*/
void  DEV_EXPORT
semaphore_destroy( semaphore_t *semaphore );

/*	semaphorestate
*	check semaphore success of failed state
*/

e_uint8 DEV_EXPORT
semaphore_state( semaphore_t *semaphore );

/*	sempost
*	Unlock a semaphore
*/

e_int32 DEV_EXPORT
semaphore_post( semaphore_t *semaphore);

/*	semwait
*	Lock a semaphore and suspend the thread if the semaphore value is zero
sem_init：初始化信号量sem_t，初始化的时候可以指定信号量的初始值，以及是否可以在多进程间共享。
sem_wait：一直阻塞等待直到信号量>0。
sem_timedwait：阻塞等待若干时间直到信号量>0。
sem_post：使信号量加1。
sem_destroy：释放信号量。和sem_init对应。
*/
e_int32 DEV_EXPORT
semaphore_wait( semaphore_t *semaphore);

e_int32 DEV_EXPORT
semaphore_timeoutwait( semaphore_t *semaphore,e_uint32 timeout_usec);

//------------------------------------------------------------------------------
//

e_int32 DEV_EXPORT
mutex_init( mutex_t *mutex );

void DEV_EXPORT
mutex_destroy( mutex_t *mutex );

e_int32 DEV_EXPORT
mutex_state( mutex_t *mutex );

e_int32 DEV_EXPORT
mutex_lock( mutex_t *mutex );

e_int32 DEV_EXPORT
mutex_timeoutlock( mutex_t *mutex,e_uint32 timeout_usec);

e_int32 DEV_EXPORT
mutex_unlock( mutex_t *mutex );

//------------------------------------------------------------------------------
int DEV_EXPORT
cond_init( condition_t *cond );

int DEV_EXPORT
cond_state( condition_t *cond );

int DEV_EXPORT
cond_signal(condition_t *cond);

int DEV_EXPORT
cond_broadcast(condition_t *cond);

int DEV_EXPORT
cond_wait(condition_t *cond, mutex_t *mutex);

int DEV_EXPORT
cond_tiemoutwait(condition_t *cond, mutex_t *mutex,e_uint32 timeout_usec);

void DEV_EXPORT
cond_destroy(condition_t *cond);


//------------------------------------------------------------------------------
//
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
//

#endif	//	_HD_THREAD_H_
// EOF: hd_thread_api.h
