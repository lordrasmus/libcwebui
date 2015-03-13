#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __GNUC__
#include <pthread.h>
#include <unistd.h>
#endif

//#include "stdafx.h"

#include "webserver_api_functions.h"
//#include "../Webserver/include/intern/list.h"
//#include "../Webserver/WebserverConfig.h"

#include "simclist.h"

#define OUT_BUFFER_SIZE 10000

#ifdef _MSC_VER
void getTime(char* buffer) {
	SYSTEMTIME systemTime;
	GetSystemTime ( &systemTime );
	sprintf ( buffer, "%02d:%02d:%02d", systemTime.wHour, systemTime.wMinute, systemTime.wSecond );
}
void mysleep(unsigned long ticks) {
	Sleep ( ticks );
}
#else
int getTime(char* buffer) {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	return sprintf(buffer, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
void mysleep(unsigned long ticks) {
	//usleep(ticks * 1000);
	//sleep(1);
	struct timespec req;
	req.tv_sec = 0;
	req.tv_nsec = 100000000;
	nanosleep(&req, 0);
}
#endif

#ifdef WEBSERVER_USE_WEBSOCKETS

list_t uhr_clients;
list_t multi_clients;

pthread_mutex_t uhr_mutex;
pthread_mutex_t multi_mutex;

#endif

void uhr_loop() {
	char buffer[1000];
#ifdef WEBSERVER_USE_WEBSOCKETS
	char buffer2[1000];
	int ret;
	char* guid;
	char* remove;
#endif

	while (1) {
		getTime(buffer);
		lockGlobalVars();
		setGlobalVar("uhr_zeit", buffer);
		unlockGlobalVars();

#ifdef WEBSERVER_USE_WEBSOCKETS
		ret = pthread_mutex_lock(&uhr_mutex);
		if (ret != 0) continue;

		sprintf(buffer2, "UhrZeit:%s", buffer);

		guid = 0;
		remove = 0;
		ws_list_iterator_start(&uhr_clients);
		while ((guid = (char*) ws_list_iterator_next(&uhr_clients))) {
			ret = WebsocketSendTextFrame(guid, buffer2, strlen(buffer2));
			if (ret == -1) {
				remove = guid;
			}
		}
		ws_list_iterator_stop(&uhr_clients);
		if (remove != 0) {
			ws_list_delete(&uhr_clients, guid);
			WebserverFree(guid);
		}

		pthread_mutex_unlock(&uhr_mutex);
#endif
		mysleep(1000);
	}
}

#ifdef WEBSERVER_USE_WEBSOCKETS

void handleCommandSocket(char *guid, char*msg) {
	char* tmp;
	if (0 == strcmp(msg, "startUhr")) {
		int ret = 0;
		ret = pthread_mutex_lock(&uhr_mutex);
		if (ret == 0) {
			tmp = WebserverMalloc(strlen(guid)+1);
			strcpy(tmp, guid);
			ws_list_append(&uhr_clients, tmp);
			pthread_mutex_unlock(&uhr_mutex);
		}
	}
	if (0 == strncmp(msg, "Echo:", 5)) {
		WebsocketSendTextFrame(guid, msg, strlen(msg));
	}
}

void deleteClient(char* guid) {
	char* tmp_guid;
	int ret = 0;
	ret = pthread_mutex_lock(&uhr_mutex);
	if (ret == 0) {
		ws_list_iterator_start(&uhr_clients);
		while ((tmp_guid = (char*) ws_list_iterator_next(&uhr_clients))) {
			if (0 == strcmp(tmp_guid, guid)) {
				printf("Client entfern           : %s CommandSocket\n", guid);
				break;
			}
		}
		ws_list_iterator_stop(&uhr_clients);
		if (tmp_guid != 0) ws_list_delete(&uhr_clients, tmp_guid);
		pthread_mutex_unlock(&uhr_mutex);
	} else {
		printf("deleteClient Commandsocket Mutex Timeout\n");
	}
}

int multis = 0;

void handleMultiSocket(char *guid) {
	char* tmp;
	int ret = 0;
	ret = pthread_mutex_lock(&multi_mutex);
	if (ret == 0) {
		tmp = WebserverMalloc(strlen(guid)+1);
		strcpy(tmp, guid);
		ws_list_append(&multi_clients, tmp);
		multis++;
		pthread_mutex_unlock(&multi_mutex);
	}
}

void multiThread(void* p) {
	int ret = 0; //, len;
	//char* guid;
	//char buffer[1000];

	//PlatformCreateMutex(&multi_mutex);
	while (1) {
		ret = pthread_mutex_lock(&multi_mutex);
		printf("Multi Send\n");

		/*liste = getFirstListNode ( uhr_clients );
		 while ( liste != 0 ) {
		 guid = ( char* ) liste->value;
		 len = snprintf ( buffer,1000,"Test%d",multis );
		 //ret = sendFrame ( guid,buffer,len );
		 if ( ret == -1 ) {
		 printf ( "Multi Thread send error   : %s\n",guid );
		 }
		 liste = getNextListNode ( liste );
		 }*/

		pthread_mutex_unlock(&multi_mutex);
		mysleep(1000);

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
	//len = snprintf(buffer, 1000, "closeconnection");
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

DEFINE_WEBSOCKET_HANDLER( "" , CommandSocket_handler_2) {
switch (signal) {
	case WEBSOCKET_MSG:
	printf("CommandSocket Handler 2 %s\n", msg);

	WebsocketSendBinaryFrame(guid, msg, strlen(msg));
	//WebsocketSendTextFrame(guid, msg, strlen(msg));

	break;
}
}

#endif

void *print_message_function(void *ptr) {
uhr_loop();
return 0;
}

void startApiThreads() {

#ifdef WEBSERVER_USE_WEBSOCKETS

ws_list_init(&uhr_clients);
#ifdef _MSC_VER
HANDLE hThread2;
DWORD dwThreadID2;

uhr_clients = 0;

hThread2 = CreateThread ( NULL, 0, UhrThread, ( LPVOID ) 0, 0, &dwThreadID2 );
CreateThread ( NULL, 0, MultiThread, ( LPVOID ) 0, 0, 0 );

#else
pthread_t thread1;
pthread_mutex_init(&uhr_mutex, 0);
pthread_mutex_init(&multi_mutex, 0);
pthread_create(&thread1, NULL, print_message_function, 0);
#endif

#endif
}

#ifdef _MSC_VER

#ifdef WEBSERVER_USE_WEBSOCKETS

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

#endif
