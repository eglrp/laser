// comtest.cpp : Defines the entry point for the console application.
//
#if TEST_MSG_MONITOR
#include "../hd_message_monitor.h"
#include <hd_thread_api.h>
#include <hd_timer_api.h>

volatile int count=MAX_CLIENT_SIZE;

void request_thread(void *data) {
	int i = 10;
	e_int32 ret;
	e_uint8 buf[128];
	e_uint8 msg[128];
	fsocket_t* fd = (fsocket_t*) data;
	sprintf(msg, "\tREQUEST MSG FROM %hd\r\n", fd->id);

	while (i--) {
		DMSG((STDOUT,"[%d] client send %s",i,msg));
		ret = fsocket_request(fd, msg, strlen(msg), buf, 128, 1000000000); //1 s
		if (ret > 0) {
			DMSG((STDOUT,"[%s.%d] client request recv [ %s ]\r\n",
					fd->name,fd->id,buf));
		} else {
			DMSG((STDOUT,"[%s.%d] client request failed\r\n",fd->name,fd->id));
		}

		Delay(1000);
	}

	fsocket_close(fd);
	count++;
}

int main(int argc, char *argv[]) {
	e_int32 ret,i;
	hd_connect_t connect;
	msg_monitor_t monitor = { 0 };
	ethread_t* threads[MAX_CLIENT_SIZE];
	e_uint8 name[128]={'t','e','s','t',0};

	ret = sc_open_socket(&connect, "127.0.0.1", 6666);
	e_assert(ret>0, ret);
	ret = sc_connect(&connect);
	e_assert(ret>0, ret);
	ret = mm_init(&monitor, &connect);
	e_assert(ret>0, ret);
	mm_start(&monitor);

#if 0
	fsocket_t* fd = mm_create_socket(&monitor, name);
	if (!fd) {
		DMSG((STDOUT,"ERROR CREATE FAKE SOCKET\r\n"));
	} else{
		request_thread(fd);
	}
#else
	while (count--) {
		sprintf(name,"Thread[%d]",count);
		fsocket_t* fd = mm_create_socket(&monitor,name);
		DMSG((STDOUT,"create request thread %d\r\n",count));
		ret = createthread("request", (thread_func) &request_thread, (void*) fd,
				NULL, &threads[count]);
		if (ret <= 0) {
			DMSG((STDOUT,"createhread failed!\r\n"));
			return E_ERROR;
		}
		ret = resumethread(threads[count]);
		if (ret <= 0) {
			DMSG((STDOUT,"resumethread failed!\r\n"));
			return E_ERROR;
		}
	}
#endif
	while(count<MAX_CLIENT_SIZE-1) {
		//DMSG((STDOUT,"COUNT = %d",count));
		sleep(1);
	}

	for (i=0;i<MAX_CLIENT_SIZE;i++) {
		killthread( threads[i]);
	}
	mm_stop(&monitor);
	sc_close(&connect);
	return 0;
}

#endif
