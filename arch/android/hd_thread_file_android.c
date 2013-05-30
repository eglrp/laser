/** pthread functions for ANDROID platform
 * author: Joy.you
 * email: yjcpui@gmail.com
 * date: 2013-05-08
 *
 * NOTE: link to android bionic libc which contains librt and libpthread
 * which is different from traditional unix-like oses
 */

#ifdef ANDROID_OS
#include <hd_timer_api.h>
#include <hd_event_api.h>
#include <hd_thread_api.h>
#include <hd_inter_thread.h>

#include <pthread.h>	/* posix threads & posix mutexes */
#include <signal.h>	/* signal kill */
#include <semaphore.h>	/* posix semaphores */
#include <stdio.h>
#include <unistd.h>	/* posix definitions */
#include <sched.h>	/* pthread priority setting */
#include <sys/time.h>

#ifdef DMSG
#undef DMSG
#define DMSG
#endif

extern thread_man_t thread_man;
//extern e_uint8 thread_mem[THREAD_MEM_SIZE];

static void _private_thread_main(ethread_t *thread) {
	while (thread->state != THREAD_START) {
		//DMSG((STDOUT,"thread in suspend state"));
		Delay(1);
	}
	DMSG((STDOUT, "thread resumed"));
	thread->state = THREAD_RUN;
	thread->func(thread->data);

	thread->state = THREAD_EXIT;
	//pthread_destroy(thread->priv);
	/* if thread pipes are created, close them and free space */
	if (thread->queue) {
		close(((int*) (thread->queue))[0]);
		close(((int*) (thread->queue))[1]);
		free((int*) thread->queue);
	}
	DMSG((STDOUT, "线程结束 thread %s", thread->name));
	/* THREAD_DEL_DATA(THREAD, &THREAD_MAN);   //doing this for what?
	 THREAD_FREEDATA(THREAD, &THREAD_MAN); */
}

/*
 * createthread
 * create thread.
 */
e_int32 createthread(const char *name, thread_func func, void *data,
		e_uint32 priority, ethread_t **thread) {
	e_int32 thread_ret = 0;
	ethread_t *this_thread = NULL;

	pthread_attr_t athread_attr = { 0 };
	struct sched_param athread_schedparam = { 0 };

	/* 1 check thread man state */
	/* 2 alloc a thread node and set data */
	/* 3 create system thread */
	/* 4 malloc thread stack space and create task */

	/* check thread pool state */
	if (g_thread_man.state != TRUE) {
		INIT_LIST_HEAD( &g_thread_man.used_list_head);
		g_thread_man.state = TRUE;
	}

	/* alloc a thread node and set data */
	E_MALLOC( ALIGN(sizeof(ethread_t)), this_thread, ethread_t);
	if (this_thread == 0x0) {
		DMSG((STDOUT, "allocating mem for thread from pool\n"));
		return 0x0;
	}
	this_thread->thread_id = 0;
	/* set thread priority */
	pthread_attr_init(&athread_attr);
	if (pthread_attr_getschedparam(&athread_attr, &athread_schedparam)) {
		return 0;
	}
	athread_schedparam.sched_priority = priority;
	if (pthread_attr_setschedparam(&athread_attr, &athread_schedparam)) {
		return 0;
	}

	/* create pthread */
	this_thread->priority = priority;
	this_thread->data = data;
	this_thread->func = func;
	this_thread->state = THREAD_PAUSE;
	strcpy(this_thread->name, name);

	if (pthread_create(&(this_thread->thread_id), &athread_attr,
			(void*) _private_thread_main, this_thread) != 0) {
		pthread_attr_destroy(&athread_attr);
		goto exit;
	}
	pthread_attr_destroy(&athread_attr);
	DMSG((STDOUT, "创建线程: %s", name));
	/* create thread communication pipe */
	int* pipe_fds = (int*) calloc(2, sizeof(int));

	if (!pipe(pipe_fds) && pipe_fds) {
		this_thread->queue = (void*) pipe_fds;
		DMSG((STDOUT, "Pipe pointer: %p", this_thread->queue));
	} else {
		DMSG((STDOUT, "Pipe failed"));
		goto exit;
	}

	list_add_before( &this_thread->list_node, &g_thread_man.used_list_head);
	*thread = this_thread;
	g_thread_man.cur_num++;

	DMSG((STDOUT, "create thread %s succeeded:(id %d,prio %d,thread:%p)\n",
			name, this_thread->thread_id, this_thread->priority, this_thread));
	return this_thread->thread_id;

	exit: if (this_thread) {
		E_FREE( this_thread);
		this_thread = NULL;
	}
	return 0;
}

/** threadstate
 * get thread state
 */
e_int32 threadstate(ethread_t *thread) {
	if (thread == 0x0)
		return 0;

	return thread->state;
}

/*
 * killthread
 * kill the specified thread.
 */
void killthread(ethread_t *thread) {
	/* 1 check thread state */
	/* 2 delete thread in system */
	/* 3 clean up itc data witch belongs to the thread */
	/* 4 free memory and thread node */

	if (!threadstate(thread) || thread == 0x0) {
		DMSG((STDOUT, "killing ethread\n"));
		return;
	}

	pthread_join(thread->thread_id,NULL);

	/* post quit event to the specified thread */
//	PostThreadEvent(thread, QUIT, 0, 0);
//	do {
//		Delay(100);
//	} while (--count > 0);
	/*	 if thread pipes are created, close them and free space
	 if(thread->queue){
	 close(((int*)(thread->queue))[0]);
	 close(((int*)(thread->queue))[1]);
	 free((int*)thread->queue);
	 }
	 */
//	free memory and thread node
	list_del( &thread->list_node);
	E_FREE( thread);

	g_thread_man.cur_num--;
	(g_thread_man.cur_num <= 0) ? (g_thread_man.cur_num = 0) : (0);
	DMSG((STDOUT, "success to destroy thread\r\n"));
}

/** suspendthread:
 * suspends the specified thread.
 */
e_int32 suspendthread(ethread_t *thread) {
	if (!threadstate(thread) || thread == 0x0) {
		DMSG((STDOUT, "thread has killed, or does not created yet\n"));
		return 0;
	}

	if (thread->state == THREAD_PAUSE) {
		DMSG((STDOUT, "current thread is in Suspend state\n"));
		return 1;
	}

	DMSG((STDOUT, "success to suspend thread \n"));
	thread->state = THREAD_PAUSE;
	return 1;
}

/** resumethread:
 * decrements a thread's suspend count,When the suspend count
 * is decremented to zero, the execution of the thread is resumed.
 */
e_int32 resumethread(ethread_t *thread) {

	if (!threadstate(thread) || thread == 0x0) {
		DMSG((STDOUT, "thread has killed, or not created yet\n"));
		return 0;
	}

	if (thread->state == THREAD_RUN) {
		DMSG((STDOUT, "current thread is in Running state\n"));
		return 0;
	}

	if (thread->state == THREAD_PAUSE) {
		thread->state = THREAD_START;
		DMSG((STDOUT, "success to resume thread\n"));
		return 1;
	}

	return 0;
}

/** holdthread:
 */
e_int32 holdthread(ethread_t *thread) {
	if (!threadstate(thread) || thread == 0x0) {
		DMSG((STDOUT, "thread has killed, or does not created yet\n"));
		return 0;
	}

	if (thread->state == THREAD_HOLD) {
		DMSG((STDOUT, "current thread is in Holding state\n"));
		return 1;
	}

	if (thread->state == THREAD_START) {
		DMSG((STDOUT, "current thread is in ready state\n"));
		return 1;
	}
	DMSG((STDOUT, "success to hold thread\n"));

	thread->state = THREAD_HOLD;

	PostThreadEvent(thread, QUIT, 0, 0);
	return 1;
}

/** getthreadid
 * get current thread id
 */
e_int32 getthreadid(ethread_t *thread) {
	if (!threadstate(thread))
		return 0;

	return thread->thread_id;
}

/*	thread_should_quit
 *		check if thread should quit
 */
e_int32 threadshouldquit(ethread_t *thread) {
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
e_int32 threadwait(ethread_t *thread, e_uint32 timeout) {
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

/** semaphore_state
 * check semaphore success of failed state
 */
e_uint8 semaphore_state(semaphore_t *semaphore) {
	return (semaphore->state == 1) ? 1 : 0;
}

/** createsemaphore
 * create a semaphore
 */
e_uint8 semaphore_init(semaphore_t *semaphore, e_int32 init_value) {
	sem_t *psem = calloc(1, sizeof(sem_t));
	/* 1 create system semaphore */
	/* 2 set semaphore value */
	if (init_value < 0) {
		DMSG((STDOUT, "system semaphore exist\n"));
		return 0;
	}

	BZERO(semaphore, semaphore_t);

	/* create system semaphore */
	/* set semaphore value */
	if (!sem_init(psem, 0, init_value)) {
		semaphore->priv = psem;
		semaphore->init_value = init_value;
		semaphore->state = 1;

		DMSG((STDOUT, "success to create system semaphore\n"));
		return 1;
	}
	return 0;
}

/** destroysemaphore
 * destroy a semaphore
 */
void semaphore_destroy(semaphore_t *semaphore) {
	if (!semaphore_state(semaphore)) {
		DMSG((STDOUT, "fail to destroy a semaphore\n"));
		return;
	}

	sem_destroy((sem_t *) semaphore->priv);
	free(semaphore->priv);

	DMSG((STDOUT, "succeed to destroy system semaphore\n"));
	BZERO(semaphore, semaphore_t);
}

/** sempost
 * Unlock a semaphore
 */
e_int32 semaphore_post(semaphore_t *semaphore) {
	e_int32 ret;
	if (!semaphore_state(semaphore)) {
		DMSG((STDOUT, "post an empty or destroyed semaphore\n"));
		return 0;
	}
	ret = sem_post((sem_t*) semaphore->priv);
	if (ret) {
		DMSG((STDOUT, "sem_post  failed\n"));
		return 0;
	}
	DMSG((STDOUT, "succeed to post semaphore\n"));
	return 1;
}

/** semwait
 * Lock a semaphore and suspend the thread if the semaphore value is zero
 */
e_int32 semaphore_wait(semaphore_t *semaphore) {
	int ret;
	if (!semaphore_state(semaphore)) {
		DMSG((STDOUT, "wait on an empty or destroyed semaphore\n"));
		return 0;
	}

	ret = sem_wait((sem_t*) semaphore->priv);
	if (ret) {
		DMSG((STDOUT, "sem_wait  failed\n"));
		return 0;
	}
	DMSG((STDOUT, "succeed to wait semaphore\n"));

	return 1;
}

e_int32 semaphore_timeoutwait(semaphore_t *semaphore, e_uint32 timeout_usec) {
	int ret;
	struct timeval now;
	struct timespec outtime;
	if (!semaphore_state(semaphore)) {
		DMSG((STDOUT, "semaphore_timeoutwait-->sem does not init yet\n"));
		return 0;
	}
	gettimeofday(&now, NULL);
	outtime.tv_sec = now.tv_sec + timeout_usec / (1000 * 1000);
	outtime.tv_nsec = (now.tv_usec + timeout_usec % (1000 * 1000)) * 1000;

	outtime.tv_sec += outtime.tv_nsec / (1000*1000*1000);
	outtime.tv_nsec %= (1000*1000*1000);

	ret = sem_timedwait(semaphore->priv, &outtime);
	if (ret) //failed
	{
		DMSG((STDOUT, "timeout wait semaphore failed,errorcode:%d\n",ret));
		return 0;
	}
	DMSG((STDOUT, "success to wait semaphore\n"));
	return 1;
}

/** createmutex
 * create a mutex
 */
e_int32 mutex_init(mutex_t *mutex) {
	e_int32 ret;
	pthread_mutex_t *pmutex = calloc(1, sizeof(pthread_mutex_t));

	/* 1 create system mutex */
	BZERO( mutex, mutex_t);
	/* 2 set the data */
	ret = pthread_mutex_init(pmutex, 0x0);
	mutex->priv = pmutex;
	mutex->state = 1;
	DMSG((STDOUT, "mutex created:%p", mutex->priv));
	return ret ? 0 : 1;
}

/** mutex_state
 * query state of mutex
 */
e_int32 mutex_state(mutex_t *mutex) {
	return (mutex->state == 1) ? 1 : 0;
}

/** destroymutex
 * destroy a mutex
 */
void mutex_destroy(mutex_t *mutex) {
	if (!mutex_state(mutex)) {
		DMSG((STDOUT, "mutex hasn't been inited yet\n"));
		return;
	}

	DMSG((STDOUT, "destroying pthread mutex %p", mutex->priv));
	pthread_mutex_destroy(mutex->priv);
	free(mutex->priv);

	BZERO(mutex, mutex_t);
	DMSG((STDOUT, "destroy mutex successfully\n"));
}

/** mutexlock
 * lock mutex
 */
e_int32 mutex_lock(mutex_t *mutex) {
	e_int32 ret;
	if (!mutex_state(mutex)) {
		DMSG((STDOUT, "mutexlock->mutex does not init yet\n"));
		return 0;
	}

	ret = pthread_mutex_lock(mutex->priv);
	if (ret) {
		DMSG((STDOUT, "failed to lock mutex\n"));
		return 0;
	}
	DMSG((STDOUT, "success to lock mutex\n"));
	return 1;
}

e_int32
mutex_timeoutlock(mutex_t *mutex, e_uint32 timeout_usec)
{
	int ret;
	struct timeval now;
	struct timespec outtime;
	if (!mutex_state(mutex)) {
		DMSG((STDOUT, "mutex_timeoutlock->mutex does not init yet\n"));
		return 0;
	}
	gettimeofday(&now, NULL);
	outtime.tv_sec = now.tv_sec + timeout_usec / (1000 * 1000);
	outtime.tv_nsec = (now.tv_usec + timeout_usec % (1000 * 1000)) * 1000;

	outtime.tv_sec += outtime.tv_nsec / (1000*1000*1000);
	outtime.tv_nsec %= (1000*1000*1000);

	ret = pthread_mutex_timedlock(mutex->priv, &outtime);
	if (ret) //failed
	{
		DMSG((STDOUT, "timeout wait mutex failed\n"));
		return 0;
	}
	DMSG((STDOUT, "success to wait mutex\n"));
	return 1;
}

/** mutexunlock
 * unlock mutex
 */
e_int32 mutex_unlock(mutex_t *mutex) {
	int ret;
	if (!mutex_state(mutex)) {
		DMSG((STDOUT, "mutexunlock-->>mutex does not init yet\n"));
		return 0;
	}

	ret = pthread_mutex_unlock(mutex->priv);
	if (ret) {
		DMSG((STDOUT, "pthread_mutex_unlock  failed\n"));
		return 0;
	}
	DMSG((STDOUT, "success to unlock mutex\n"));
	return 1;
}

//------------------------------------------------------------------------------
int cond_init(condition_t *cond) {
	pthread_cond_t *pcond = calloc(1, sizeof(pthread_cond_t));

	/* 1 create system cond */
	BZERO( cond, condition_t);
	/* 2 set the data */
	pthread_cond_init(pcond, NULL);
	cond->priv = pcond;
	cond->state = 1;
	DMSG((STDOUT, "cond created:%p", cond->priv));
	return 1;
}

int cond_state(condition_t *cond) {
	return (cond->state == 1) ? 1 : 0;
}

int cond_signal(condition_t *cond) {
	int ret;
	if (!cond_state(cond)) {
		DMSG((STDOUT, "cond_signal-->cond does not init yet\n"));
		return 0;
	}

	ret = pthread_cond_signal(cond->priv);
	if (ret) {
		DMSG((STDOUT, "pthread_cond_signal  failed\n"));
		return 0;
	}
	DMSG((STDOUT, "success to singnal cond\n"));
	return 1;
}

int cond_broadcast(condition_t *cond) {
	int ret;
	if (!cond_state(cond)) {
		DMSG((STDOUT, "cond_signal-->cond does not init yet\n"));
		return 0;
	}

	pthread_cond_broadcast(cond->priv);
	DMSG((STDOUT, "success to broadcast cond\n"));
	return 1;
}

int cond_wait(condition_t *cond, mutex_t *mutex) {
	int ret;
	if (!cond_state(cond)) {
		DMSG((STDOUT, "cond_signal-->cond does not init yet\n"));
		return 0;
	}

	ret = pthread_cond_wait(cond->priv, mutex->priv);
	if (ret) {
		DMSG((STDOUT, "pthread_cond_wait  failed\n"));
		return 0;
	}
	DMSG((STDOUT, "success to wait cond\n"));
	return 1;
}

int cond_tiemoutwait(condition_t *cond, mutex_t *mutex, e_uint32 timeout_usec) {
	int ret;
	struct timeval now;
	struct timespec outtime;
	if (!cond_state(cond)) {
		DMSG((STDOUT, "cond_signal-->cond does not init yet\n"));
		return 0;
	}
	gettimeofday(&now, NULL);
	outtime.tv_sec = now.tv_sec + timeout_usec / (1000 * 1000);
	outtime.tv_nsec = (now.tv_usec + timeout_usec % (1000 * 1000)) * 1000;

	outtime.tv_sec += outtime.tv_nsec / (1000*1000*1000);
	outtime.tv_nsec %= (1000 * 1000 * 1000);

	ret = pthread_cond_timedwait(cond->priv, mutex->priv, &outtime);
	if (ret) //failed
	{
		DMSG((STDOUT, "timeout wait cond failed\n"));
		return 0;
	}
	DMSG((STDOUT, "success to wait cond\n"));
	return 1;
}

void cond_destroy(condition_t *cond) {
	if (!cond_state(cond)) {
		DMSG((STDOUT, "cond hasn't been inited yet\n"));
		return;
	}

	DMSG((STDOUT, "destroying pthread cond %p", cond->priv));
	pthread_cond_destroy(cond->priv);
	free(cond->priv);

	BZERO(cond, condition_t);
	DMSG((STDOUT, "destroy cond successfully\n"));
}

#endif /* ANDROID_OS */
