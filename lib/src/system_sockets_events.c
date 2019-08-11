/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#include "webserver.h"

/*
 http://tangentsoft.net/wskfaq/articles/bsd-compatibility.html
 http://publib.boulder.ibm.com/infocenter/iseries/v5r3/index.jsp?topic=/rzab6/rzab6xnonblock.htm
 http://www.wangafu.net/~nickm/libevent-book/01_intro.html

 libevent stuertzt ab wenn man das event flag von einem beendeten socket veraendert ( struct event )

*/

#ifdef _WEBSERVER_MEMORY_DEBUG_
    extern int print_blocks_now;
#endif

#ifdef USE_LIBEVENT

typedef unsigned char u_char;
#include <event2/event.h>
#include <event2/thread.h>


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

struct event_base* base;

void eventHandler(int a, short b, void *t) {
	char pers = 0;

	#ifdef _WEBSERVER_MEMORY_DEBUG_
		if ( print_blocks_now == 1 ){
			printBlocks();
			print_blocks_now = 0;
		}
	#endif
	
	short tmp = b & ~( EV_TIMEOUT | EV_READ | EV_WRITE | EV_SIGNAL | EV_PERSIST | EV_ET ) ;
	
	if ( tmp != 0 ){
		/*
		 Dürfte nie auftreten, es scheint das eine Event für eine bereits freigegeben Struktur aufgerufen wird
		 möglicherweise Problem mit Websockets und / oder OpenSSL
		*/
		printf("BUG: Invalid Flags 0x%X\n",tmp);
		return;
	}


	if ((b & EV_PERSIST) == EV_PERSIST) {
		pers = EVENT_PERSIST;
	}
	
	if ((b & EV_SIGNAL) == EV_SIGNAL) {
		printf("SIGNAL ignored a: %d  b : %d  t: 0x%p \n",a,b,t);
		/* handleer(a, EVENT_SIGNAL | pers, t); */
		fflush(stdout);
		return;
	}

	if ((b & EV_TIMEOUT) == EV_TIMEOUT) {
		handleer(a, EVENT_TIMEOUT | pers, t);
		fflush(stdout);
		return;
	}

	if ((b & EV_READ) == EV_READ) {
		handleer(a, EVENT_READ | pers, t);
		fflush(stdout);
		return;
	}

	if ((b & EV_WRITE) == EV_WRITE) {
		handleer(a, EVENT_WRITE | pers, t);
		fflush(stdout);
		return;
	}

	

}

void addEventSocketRead(socket_info* sock) {
#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Read  ( SSL Blocked )","");
#endif
		sock->ssl_event_flags = EV_READ;
		return;
	}
#endif

#if _WEBSERVER_HANDLER_DEBUG_ > 4
	LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Read ","");
#endif
	
	if (sock->my_ev == 0){
		sock->my_ev = event_new(base, sock->socket, EV_READ, eventHandler, sock);
	}else{
		event_assign(sock->my_ev, base, sock->socket, EV_READ, eventHandler, sock);
	}
	event_add(sock->my_ev, NULL);

}

void addEventSocketReadPersist(socket_info* sock) {
#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Read Persist ( SSL Blocked )","");
#endif
		sock->ssl_event_flags = EV_READ | EV_PERSIST;
		return;
	}
#endif

#if _WEBSERVER_HANDLER_DEBUG_ > 4
	LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Read Persist","");
#endif
	
	if (sock->my_ev == 0){
		sock->my_ev = event_new(base, sock->socket, EV_READ | EV_PERSIST, eventHandler, sock);
	}else{
		event_assign(sock->my_ev, base, sock->socket, EV_READ | EV_PERSIST, eventHandler, sock);
	}
	event_add(sock->my_ev, NULL);

}

void addEventSocketWrite(socket_info* sock) {

#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Write ( SSL Blocked )","");
#endif
		sock->ssl_event_flags = EV_WRITE;
		return;
	}
#endif
	
#if _WEBSERVER_HANDLER_DEBUG_ > 4
	LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Write","");
#endif
	
	if (sock->my_ev == 0){
		sock->my_ev = event_new(base, sock->socket, EV_WRITE, eventHandler, sock);
	}else{
		event_assign(sock->my_ev, base, sock->socket, EV_WRITE, eventHandler, sock);
	}
	event_add(sock->my_ev, NULL);

}

void addEventSocketWritePersist(socket_info* sock) {

#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Write Persist ( SSL Blocked )","");
#endif

		sock->ssl_event_flags = EV_WRITE | EV_PERSIST;
		return;
	}
#endif

#if _WEBSERVER_HANDLER_DEBUG_ > 4
	LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Write Persist","");
#endif

	
	if (sock->my_ev == 0){
		sock->my_ev = event_new(base, sock->socket, EV_WRITE | EV_PERSIST, eventHandler, sock);
	}else{
		event_assign(sock->my_ev, base, sock->socket, EV_WRITE | EV_PERSIST, eventHandler, sock);
	}
	event_add(sock->my_ev, NULL);

}

void addEventSocketReadWritePersist(socket_info* sock) {

#ifdef WEBSERVER_USE_SSL
	if(sock->ssl_block_event_flags == 1){
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Read/Write Persist ( SSL Blocked )","");
#endif
		sock->ssl_event_flags = EV_READ | EV_WRITE | EV_PERSIST;
		return;
	}
#endif
	
#if _WEBSERVER_HANDLER_DEBUG_ > 4
	LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Add Event Read/Write Persist","");
#endif
	
	if (sock->my_ev == 0){
		sock->my_ev = event_new(base, sock->socket, EV_READ | EV_WRITE | EV_PERSIST, eventHandler, sock);
	}else{
		event_assign(sock->my_ev, base, sock->socket, EV_READ | EV_WRITE | EV_PERSIST, eventHandler, sock);
	}
	event_add(sock->my_ev, NULL);

}

#ifdef WEBSERVER_USE_SSL

void commitSslEventFlags( socket_info* sock ) {

	sock->ssl_block_event_flags = 0;
	
	if (sock->my_ev == 0){
		sock->my_ev = event_new(base, sock->socket, sock->ssl_event_flags, eventHandler, sock);
	}else{
		event_assign(sock->my_ev, base, sock->socket, sock->ssl_event_flags, eventHandler, sock);
	}
	event_add(sock->my_ev, NULL);
#if _WEBSERVER_HANDLER_DEBUG_ > 4
	LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"commit SSL Event Flags Persist","");
#endif
}

#endif

void delEventSocketWritePersist(socket_info* sock) {
	
	int ret;
	if (sock->my_ev != 0) {
		ret = event_del(sock->my_ev);
		if (ret != 0) {
			LOG(HANDLER_LOG, NOTICE_LEVEL, sock->socket, "%s","Del Event Write Persist Failed");
		}
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Del Event Write Persist","");
#endif
	} else {
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"%s","Del Event Write Persist  Error no ev structure");
#endif
	}
}

void delEventSocketReadPersist(socket_info* sock) {
	delEventSocketWritePersist(sock);
	/*int ret;
	 if(sock->my_ev != 0){
	 event_del(sock->my_ev);
	 #ifdef _WEBSERVER_HANDLER_DEBUG_
	 LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Del Event Read Persist","");
	 #endif
	 }else{
	 #ifdef _WEBSERVER_HANDLER_DEBUG_
	 LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"Del Event Read Persist  Error no ev structure","");
	 #endif
	 }*/
}

void delEventSocketAll(socket_info* sock) {
	int ret;
	if (sock->my_ev != 0) {
				
		ret = event_del(sock->my_ev);
				
		if ( event_pending(sock->my_ev, EV_TIMEOUT | EV_READ | EV_WRITE | EV_SIGNAL, NULL ) ){
			LOG(HANDLER_LOG,ERROR_LEVEL,sock->socket,"%s","Still pending events");
		}
		
		if (ret != 0) {
			LOG(HANDLER_LOG, ERROR_LEVEL, sock->socket,"%s", "Del All Events Failed");
		}
#if _WEBSERVER_HANDLER_DEBUG_ > 4
		LOG(HANDLER_LOG,NOTICE_LEVEL,sock->socket,"%s","Del All Events");
#endif
	} else {
		LOG(HANDLER_LOG,ERROR_LEVEL,sock->socket,"%s","Del All Events Failed  Error no ev structure");
	}
}

/*char checkEventSocket(socket_info* sock) {
	struct timeval tv;
	return event_pending(sock->my_ev, EV_TIMEOUT | EV_READ | EV_WRITE | EV_SIGNAL, NULL );
}*/

void deleteEvent(socket_info* sock){
	if ( sock->my_ev != 0){
		event_free(sock->my_ev);
	}
	sock->my_ev=0;
}

void initEvents( void ) {
	const char ** list;
	char buffer[200];
	int pos = 0, i = 0;
	struct event_config *config;

	event_enable_debug_mode();

#ifdef __GNUC__
	evthread_use_pthreads();
#else
	evthread_use_windows_threads();
#endif

	config = event_config_new();
	/* event_config_avoid_method(config, "select"); */
	/* We want a method that can work with non-socket file descriptors */
	/* event_config_require_features(config, EV_FEATURE_FDS); */
	base = event_base_new_with_config(config);
	if (!base) {
		/* There is no backend method that does what we want. */
		LOG(MESSAGE_LOG, NOTICE_LEVEL, 0, "%s","libEvent error creating base");
		exit(1);
	}
	event_config_free(config);

	list = event_get_supported_methods();

	while (list[i] != 0) {
		pos += snprintf(&buffer[pos], 200 - pos, "%s ", list[i]);
		i++;
	}

	free(list);

	/* LOG(MESSAGE_LOG, NOTICE_LEVEL, 0, "libEvent %s using %s ( Methods: %s )", event_get_version() , event_get_method() , buffer); */
	LOG(MESSAGE_LOG, NOTICE_LEVEL, 0, "libEvent %s ( Methods: %s )", event_get_version() , buffer);

}

char waitEvents( void ) {
	return event_base_dispatch(base);
}

void breakEvents(void){
	event_base_loopbreak(base);
}

void freeEvents( void ) {
	event_base_free(base);
}






#endif

