#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "webserver_api_functions.h"


#ifdef WEBSERVER_USE_WEBSOCKETS


#include "simclist.h"

#define OUT_BUFFER_SIZE 10000

/*******************************************************************
 * 
 *            Helper Functions
 * 
 *******************************************************************/

int getTime(char* buffer) {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	return sprintf(buffer, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void mysleep(unsigned long ticks) {
	struct timespec req;
	req.tv_sec = 0;
	req.tv_nsec = 100000000;
	nanosleep(&req, 0);
}

/*******************************************************************
 * 
 *            Global Variables
 * 
 *******************************************************************/

list_t clock_clients;

pthread_mutex_t clock_mutex;


void uhr_loop() {
	char buffer[1000];

	char buffer2[1000];
	int ret;
	char* guid;
	char* remove;


	while (1) {
		getTime(buffer);
		
		if ( 0 == lockGlobalVars() ){
			setGlobalVar("time", buffer);
			if ( 0 != unlockGlobalVars() ){
				printf("error unlocking global var lock\n");
				exit(1);
			}
		}


		pthread_mutex_lock(&clock_mutex);
		
		sprintf(buffer2, "time: %s", buffer);

		guid = 0;
		remove = 0;
		
		// iterate connected clients
		ws_list_iterator_start(&clock_clients);		
		while ((guid = (char*) ws_list_iterator_next(&clock_clients))) {
			
			// send text frame to client
			ret = WebsocketSendTextFrame(guid, buffer2, strlen(buffer2));
			
			// mark client for removal if send fails
			if (ret == -1) {
				remove = guid;
			}
		}		
		ws_list_iterator_stop(&clock_clients);
		
		if (remove != 0) {
			ws_list_delete(&clock_clients, guid);
			WebserverFree(guid);
		}

		pthread_mutex_unlock(&clock_mutex);

		mysleep(1000);
	}
}



void handleCommandSocket(char *guid, const char *msg) {
	char* tmp;
	if (0 == strcmp(msg, "connect_clock")) {
		int ret = 0;
		ret = pthread_mutex_lock(&clock_mutex);
		if (ret == 0) {
			tmp = WebserverMalloc(strlen(guid)+1);
			strcpy(tmp, guid);
			ws_list_append(&clock_clients, tmp);
			pthread_mutex_unlock(&clock_mutex);
		}
	}
	if (0 == strncmp(msg, "echo:", 5)) {
		
		if ( 0 == strcmp( msg, "echo:error" ) ){
			char *msg2 = msg;
			msg2[3] = 0x88;
		}
		
		WebsocketSendTextFrame(guid, msg, strlen(msg));
	}
	
	if (0 == strcmp(msg, "ping")) {
		WebsocketSendTextFrame(guid, "pong", strlen("pong"));
	}
}

void deleteClient(char* guid) {
	char* tmp_guid;
	int ret = 0;
	ret = pthread_mutex_lock(&clock_mutex);
	if (ret == 0) {
		ws_list_iterator_start(&clock_clients);
		
		while ((tmp_guid = (char*) ws_list_iterator_next(&clock_clients))) {
			if (0 == strcmp(tmp_guid, guid)) {
				printf("Client entfern           : %s CommandSocket\n", guid);
				break;
			}
		}
		ws_list_iterator_stop(&clock_clients);
		
		if (tmp_guid != 0) ws_list_delete(&clock_clients, tmp_guid);
		
		pthread_mutex_unlock(&clock_mutex);
	} else {
		printf("deleteClient Commandsocket Mutex Timeout\n");
	}
}


void simpleThread(void* p) {
	char buffer[1000];
	int i, num = 1, len;
	int ret = 0;
	char *guid = (char*) p;

	for (i = 1; i < 61; i++) {
		len = snprintf(buffer, 1000, "Test%d", num++);
		ret = WebsocketSendTextFrame(guid, buffer, len);
		if (ret == -1) {
			printf("Test Thread send error   : %s\n", guid);
			WebsocketSendCloseFrame(guid);
			WebserverFree(guid);
			return;
		}
		mysleep(1000);
	}
	

	printf("Websocket API Close      : %s simple\n", guid);
	WebsocketSendCloseFrame(guid);
	WebserverFree(guid);
	return;
}

void *simple_function(void *ptr) {
	simpleThread(ptr);
	return 0;
}

DEFINE_WEBSOCKET_HANDLER( "simple" , simple_handler ) {
	pthread_t thread1;
	char *tmp;
	switch (signal) {
	case WEBSOCKET_CONNECT:
		printf("Websocket API Connect    : %s simple\n", guid);
		tmp = WebserverMalloc(strlen(guid)+1);
		strcpy(tmp, guid);
		pthread_create(&thread1, NULL, simple_function, tmp);
		break;
	}
}

DEFINE_WEBSOCKET_HANDLER( "CommandSocket" , CommandSocket_handler) {
	
	char *tmp;
	tmp = WebserverMalloc(strlen(guid)+1);
	strcpy(tmp, guid);
	
	switch (signal) {
		
		case WEBSOCKET_MSG:
			printf("CommandSocket Handler 1 %s\n", msg);
			handleCommandSocket(tmp, msg);
		break;
		
		case WEBSOCKET_DISCONNECT:
			deleteClient(tmp);
		break;
	}
	
	WebserverFree(tmp);
}



void *clock_start_helper(void *ptr) {
	uhr_loop();
	return 0;
}

void startApiThreads() {

	ws_list_init(&clock_clients);
	
	pthread_t thread1;
	pthread_mutex_init(&clock_mutex, 0);
	pthread_create(&thread1, NULL, clock_start_helper, 0);

}

#ifdef _MSC_VER



//HANDLE uhr_clients_mutex;

DWORD WINAPI UhrThread ( LPVOID Data ) {
uhr_loop();
return 0;
}

DWORD WINAPI ThreadFunc ( LPVOID Data ) {
simpleThread(Data);
return 0;
}

DWORD WINAPI MultiThread ( LPVOID Data ) {
multiThread(Data);
return 0;
}



#endif

#endif // WEBSERVER_USE_WEBSOCKETS
