//-----------------------------------------------------------------------------
//	file:	hd_thread_api.c
//	author:	Joy.you
//	date:		2013-05-08
//
//------------------------------------------------------------------------------

#ifdef MS_WIN32
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#include <hd_timer_api.h>
#include <hd_thread_api.h>

//	MS_WIN32 system function
#include <windows.h>
#include <winbase.h>

#undef DMSG
#define DMSG(_args_)

//---------------------------------------------------------------------
//	global data
#define SEM_MAX_NUM	0x7FFFFFFF

//------------------------------------------------------------------------------
//	create thread
uint32_t
createthread( const char *name, thread_func func, void *data, uint32_t priority, ethread_t **thread )
{
	ethread_t *this_thread = NULL;

	//	alloc a thread node and set data
	E_MALLOC( ALIGN(sizeof(ethread_t)), this_thread, ethread_t );
	if( this_thread == NULL ) {
		DMSG((STDOUT,"create thread failed, does not has enough thread man space\r\n"));
		return 0;
	}

	//	create system thread
	BZERO( this_thread, ethread_t );
	this_thread->func = func;
	this_thread->priority = priority;
	sprintf( this_thread->name, "%s", name );

	//this_thread->queue = CreateEvent( 0x0, FALSE, FALSE, 0x0 );
	this_thread->priv = CreateThread( 0x0,0,(LPTHREAD_START_ROUTINE)func,data,
			CREATE_SUSPENDED,(LPDWORD)&this_thread->thread_id);
	if( this_thread->priv == INVALID_HANDLE_VALUE ) {
		DMSG((STDOUT,"system create thread failed\r\n"));
		//CloseHandle( this_thread->queue );
		goto exit;
	}

	this_thread->state = EMAP_THREAD_PAUSE;
	*thread = this_thread;
	DMSG((STDOUT,"system create thread succeeded: (id %d, prio %d) \r\n",this_thread->thread_id, this_thread->priority ));
	return 1;

	exit:
	if( this_thread ) {
		E_FREE( this_thread );
		this_thread = NULL;
	}
	return 0;
}

//------------------------------------------------------------------------------
//	kill thread
void
killthread( ethread_t *thread )
{
	int count=3;
	//	1 check thread state
	//	2 delete thread in system
	//	3 free memory and thread node

	//	check thread state
	if( !threadstate( thread ) || thread == 0x0 ) {
		DMSG((STDOUT,"thread has killed, or does not created yet\r\n"));
		return;
	}

	//	delete thread in system
	//SetWaitCursor();
	/*PostThreadEvent( thread, EMAP_QUIT, 0, 0 );
	 do{
	 Delay( 100 );
	 }while(--count>0);
	 */
	thread->state = EMAP_THREAD_EXIT;
	CloseHandle( thread->priv );
	//CloseHandle( thread->queue );

	//	free memory and thread node
	E_FREE( thread );
	//RestoreCursor();
	DMSG((STDOUT,"success to destroy thread\r\n"));
}

/*
 *	suspendthread:
 * suspends the specified thread.
 */
uint32_t
suspendthread( ethread_t *thread )
{
	//	check thread state
	if( !threadstate( thread ) || thread == 0x0 ) {
		DMSG((STDOUT,"thread has killed, or does not created yet\r\n"));
		return 0;
	}

	DMSG((STDOUT,"success to suspend thread\r\n"));
	if( thread->state == EMAP_THREAD_PAUSE )
	return 1;

	thread->state = EMAP_THREAD_PAUSE;
	return SuspendThread( thread->priv );
}

/*
 *	resumethread:
 * decrements a thread's suspend count,When the suspend count
 * is decremented to zero, the execution of the thread is resumed.
 */
uint32_t
resumethread( ethread_t *thread )
{
	//	check thread state
	if( !threadstate( thread ) || thread == 0x0 ) {
		DMSG((STDOUT,"thread has killed, or does not created yet\r\n"));
		return 0;
	}

	DMSG((STDOUT,"success to resume thread\r\n"));
	if( thread->state == EMAP_THREAD_RUN )
	return 1;

	thread->state = EMAP_THREAD_RUN;
	return ResumeThread( thread->priv );
}

/*
 *	holdthread:
 */

uint32_t
holdthread( ethread_t *thread )
{
	//	check thread state
	if( !threadstate( thread ) || thread == 0x0 ) {
		DMSG((STDOUT,"thread has killed, or does not created yet\r\n"));
		return 0;
	}

	DMSG((STDOUT,"success to hold thread\r\n"));
	if( thread->state == EMAP_THREAD_HOLD ||
			thread->state == EMAP_THREAD_START ) {
		DMSG((STDOUT,"hold or not run\r\n"));
		return 1;
	}

	if( thread->state == EMAP_THREAD_PAUSE ) {
		DMSG((STDOUT,"active thread\r\n"));
		resumethread( thread );
	}

	//PostThreadEvent( thread, EMAP_QUIT, 0, 0 );
	thread->state = EMAP_THREAD_HOLD;
	return 1;
}

/*	threadstate
 *		get thread state
 */
uint32_t
threadstate( ethread_t *thread )
{
	if( thread == 0x0 )
	return 0;

	return thread->state;
}

/*	getthreadid
 *		get current thread id
 */
uint32_t
getthreadid( ethread_t *thread)
{
	if( !threadstate(thread) )
	return 0;

	return thread->thread_id;
}

/*	thread_should_quit
 *		check if thread should quit
 */
e_uint32 threadshouldquit(ethread_t *thread) {
	if (!thread)
	return 1;
	switch (thread->state) {
		case THREAD_START:
		case THREAD_RUN:
		case THREAD_PAUSE:
		case TRHEAD_RESUME:
		return 0;
		case THREAD_HOLD:
		case THREAD_EXIT:
		return 1;
		default:
		return 1;
	}
}

/*	threadwait
 *		wait for thread for quit
 */
e_uint32
threadwait(ethread_t *thread, e_uint32 timeout) {
	if (!thread)
	return 0;
	if (timeout == 0)
	timeout = 5000; //5 seconds

	timeout /= 1000;

	while (thread->state != THREAD_EXIT && timeout--) {
		//DMSG((STDOUT,"thread in suspend state"));
		Delay(1);
	}
	if (thread->state == THREAD_EXIT)
	return 1;
	return 0;
}

/*	semaphorestate
 *	check semaphore success of failed state
 */

uint8_t
semaphorestate( semaphore_t *semaphore )
{
	return (semaphore->state==1)?1:0;
}

/*	createsemaphore
 *		create a semaphore
 */
uint8_t
createsemaphore( int32 init_value, semaphore_t *semaphore )
{
	BZERO(semaphore, semaphore_t );
	semaphore->priv = CreateSemaphore( 0x0, init_value, SEM_MAX_NUM,_T(""));
	if( semaphore->priv == 0x0 )
	DMSG((STDOUT,"create system semaphore failed\r\n"));
	return 0;

	semaphore->state = 1;
	DMSG((STDOUT,"success to create system semaphore\r\n"));
	return 1;
}

/*	destroysemaphore
 *		destroy a semaphore
 */
void
destroysemaphore( semaphore_t *semaphore )
{
	if( !semaphorestate(semaphore) ) {
		DMSG((STDOUT,"can not destroy an empty or destroyed semaphore\r\n"));
		return;
	}

	DMSG((STDOUT,"success to destroy system semaphore\r\n"));
	CloseHandle( semaphore->priv );
	BZERO(semaphore, semaphore_t );
}

/*	sempost
 *	Unlock a semaphore
 */

int32
sempost( semaphore_t *semaphore, int count )
{
	if( !semaphorestate(semaphore) ) {
		DMSG((STDOUT,"can not do implemtation on an empty or destroyed semaphore\r\n"));
		return 0;
	}

	DMSG((STDOUT,"success to do semaphore implemtation: increment\r\n"));
	(count<=0)?(count=1):(0);
	return ReleaseSemaphore( semaphore->priv, count, 0x0 );
}

/*	semwait
 *	Lock a semaphore and suspend the thread if the semaphore value is zero
 */
int32
semwait( semaphore_t *semaphore, uint32_t flag )
{
	uint32_t _type;
	if( !semaphorestate(semaphore) ) {
		DMSG((STDOUT,"can not do implemtation on an empty or destroyed semaphore\r\n"));
		return 0;
	}

	_type = (flag==EMAP_SEMA_IGNORE)?IGNORE:INFINITE;
	DMSG((STDOUT,"success to do semaphore implemtation: decrement\r\n"));
	_type = WaitForSingleObject( semaphore->priv, _type );
	return 1;
}

uint32_t
createmutex( mutex_t *mutex )
{
	//	1 create system mutex
	BZERO( mutex, mutex_t );

	mutex->priv = CreateEvent( NULL, FALSE, FALSE, NULL );
	if( mutex->priv == 0x0 || mutex->priv == INVALID_HANDLE_VALUE ) {
		DMSG((STDOUT,"failed to create system mutex object\r\n"));
		return 0;
	}
	//	valid first time call
	SetEvent( mutex->priv );
	mutex->state = 1;
	DMSG((STDOUT,"system create mutex succeeed\r\n"));
	return 1;
}

void
destroymutex( mutex_t *mutex )
{
	if(!mutexstate(mutex)) {
		DMSG((STDOUT,"mutex does not init yet\r\n"));
		return;
	}

	DMSG((STDOUT,"system destroy mutex succeeed\r\n"));
	CloseHandle( mutex->priv );
	BZERO( mutex, mutex_t );
}

uint32_t
mutexstate( mutex_t *mutex )
{
	return (mutex->state==1)?1:0;
}

uint32_t
mutexlock( mutex_t *mutex )
{
	if(!mutexstate(mutex)) {
		DMSG((STDOUT,"mutexlock-->>mutex does not init yet\r\n"));
		return 0;
	}

	DMSG((STDOUT,"success to lock mutex\r\n"));
	WaitForSingleObject( mutex->priv, INFINITE );
	return 1;
}

uint32_t
mutexunlock( mutex_t *mutex )
{
	if(!mutexstate(mutex)) {
		DMSG((STDOUT,"mutexunlock-->>mutex does not init yet\r\n"));
		return 0;
	}

	DMSG((STDOUT,"success to unlock mutex\r\n"));
	SetEvent( mutex->priv );
	return 1;
}

int
cond_init( condition_t *cond )
{
	//	1 create system condition
	BZERO( cond, condition_t );

	cond->state = TRUE;
	createsemaphore( 0, &cond->sema );
	DMSG((STDOUT,"system create cond succeeed\r\n"));
	return TRUE;
}

int
cond_state( condition_t *cond )
{
	if( cond == NULL )
	return FALSE;

	return (cond->state==TRUE)?(TRUE):(FALSE);
}

int
cond_signal(condition_t *cond)
{
	// must hold the external mutex before enter
	if( cond_state(cond) == FALSE )
	return FALSE;

	if( cond->waiter > 0 ) {
		sempost( &cond->sema, 1 );
	}

	return TRUE;
}

int
cond_broadcast(condition_t *cond)
{
	// must hold the external mutex before enter
	if( cond_state(cond) == FALSE )
	return FALSE;

	if( cond->waiter > 0 ) {
		sempost( &cond->sema, cond->waiter );
	}

	return TRUE;
}

int
cond_wait(condition_t *cond, mutex_t *mutex)
{
	int ret = 0;
	// must hold the external mutex before enter
	if( cond_state(cond) == FALSE )
	return FALSE;

	cond->waiter++;
//	ret = SignalObjectAndWait( mutex->priv, cond->sema.priv, -1, FALSE );
	mutexlock( mutex );
	cond->waiter--;
	return ret == WAIT_OBJECT_0 ? TRUE : FALSE;
}

int
cond_destroy(condition_t *cond)
{
	// must hold the external mutex before enter
	if( cond_state(cond) == FALSE )
	return FALSE;

	destroysemaphore( &cond->sema );
	BZERO( cond, condition_t );
	DMSG((STDOUT,"system destroy cond succeeed\r\n"));
	return TRUE;
}

#endif // MS_WIN32
