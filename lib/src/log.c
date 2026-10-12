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

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


int last_log_length=0;

void addLog ( LogChannels channel,LogLevels level,char* filename,int fileline,const char* function,int socket,char* text,... ) {
	char buff[451];/* viel laenger darf eine logzeile nicht sein */
	int len=0,i;
	va_list ap;

	printf( "%s", "Webserver: " );

	len = snprintf (buff, 450,"%s :  ",function);
	if(len < last_log_length){
		for(i=len;i<last_log_length;i++){
			buff[i] = ' ';
		}
		len = last_log_length;
	}
	if(len > last_log_length){
		last_log_length = len;
	}
	
	buff[len] = '\0';
	printf( "%s", buff );
	len=0;

	switch ( level ){
		case DEBUG_LEVEL:
			printf(" DEBUG   : ( %d ) : ",socket);
			break;
		case NOTICE_LEVEL:
			printf(" NOTICE  : ( %d ) : ",socket);
			break;
		case INFO_LEVEL:
			printf(" INFO    : ( %d ) : ",socket);
			break;
		case WARNING_LEVEL:
			printf(" WARNING : ( %d ) : ",socket);
			break;
		case ERROR_LEVEL:
			printf(" ERROR   : ( %d ) : ",socket);
			break;
	}
	
	// Maximal 300 Zeichen vom Variablen teil ausgeben
	va_start ( ap, text );
	len = vsnprintf ( buff ,300, text, ap );
	va_end ( ap );
	if ( len < 300 ){
		buff[len] = '\0';
	}else{
		buff[300] = '\0';
	}
	printf( "%s", buff );
	
	switch ( level ){
		case DEBUG_LEVEL:
		case NOTICE_LEVEL:
			break;
		case INFO_LEVEL:
		case WARNING_LEVEL:
		case ERROR_LEVEL:
			printf( "\n   ->   File: %s, Line :%d",filename,fileline );
			break;
	}
	
	printf( "\n" );

#ifndef NUTTX    
	fflush(stdout);
#endif
}


/*
 *	Access log
 *
 *	The web server only collects what a request carried and how it was
 *	answered; where and how that is written is up to the application.
 */

static ws_access_log_handler access_log_handler = 0;

void ws_set_access_log_handler(ws_access_log_handler handler) {
	access_log_handler = handler;
}

uint64_t accessLogNowMs(void) {
#ifdef _WIN32
	return GetTickCount64();
#else
	struct timespec ts;
	if ( clock_gettime(CLOCK_MONOTONIC, &ts) != 0 ){
		return 0;
	}
	return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

void accessLogEntry(const ws_access_log_entry* entry) {
	if ( access_log_handler != 0 ){
		access_log_handler( entry );
	}
}

static const char* accessLogMethod(HttpRequestHeader* header) {
	switch ( header->method ){
		case HTTP_GET:     return "GET";
		case HTTP_POST:    return "POST";
		case HTTP_OPTIONS: return "OPTIONS";
	}
	/* rejected methods keep their name for the 405 */
	if ( header->error_method[0] != '\0' ){
		return header->error_method;
	}
	return 0;
}

void accessLogRequest(socket_info* sock) {
	ws_access_log_entry entry;
	char url[512];
	HttpRequestHeader* header = sock->header;

	if ( ( access_log_handler == 0 ) || ( header == 0 ) ){
		return;
	}

	memset( &entry, 0, sizeof( entry ) );
	entry.client_ip = sock->client_ip_str;
	entry.method = accessLogMethod( header );
	if ( header->url != 0 ){
		snprintf( url, sizeof( url ), "/%s", header->url );
		entry.url = url;
	}
	entry.status = header->response_status;
	entry.bytes = header->response_bytes;
	if ( header->request_start_ms != 0 ){
		entry.duration_ms = accessLogNowMs() - header->request_start_ms;
	}
	entry.user_agent = header->UserAgent;
	entry.referer = header->Referer;
	entry.user = header->access_log_user;
#ifdef WEBSERVER_USE_SSL
	entry.ssl = sock->use_ssl;
#endif
#ifdef WEBSERVER_USE_WEBSOCKETS
	entry.websocket = ( header->isWebsocket == 1 );
#endif

	access_log_handler( &entry );
}
