/*

libCWebUI
Copyright (C) 2012  Ramin Seyed-Moussavi

Projekt URL : http://code.google.com/p/libcwebui/

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/


#ifndef _SESSION_H_
#define _SESSION_H_

#ifdef __GNUC__
	#include "dataTypes.h"
#endif

#include "webserver.h"

typedef enum {
	SESSION_STORE		= 1,
	SESSION_STORE_SSL	= 2
} STORE_TYPES;

#define NORMAL_CHECK_OK			1
#define SSL_CHECK_OK			2
#define NOT_REGISTERED			0
#define SESSION_MISMATCH_ERROR -1

#ifdef __cplusplus
extern "C" {
#endif

void 		createSession(http_request* s,unsigned char ssl_store);

void 		initSessions(void);
void 		freeSessions(void);

void 		checkSessionCookie(http_request* s);
void        restoreSession(http_request* s,int lock_stores);



char 		setSessionValueByGUID(char* store_guid,STORE_TYPES store,const char* name,const char* value);
char 		setSessionValue(http_request* s,STORE_TYPES store,const char* name,const char* value);
ws_variable*  	getSessionValueByGUID(char* store_guid,STORE_TYPES store,const char* name);
ws_variable*  	getSessionValue(http_request* s,STORE_TYPES store,const char* name);
ws_variable* addSessionValue(http_request* s, STORE_TYPES store, const char* name);
void 		printSessionValue(http_request* s,STORE_TYPES store,char* name);
char 		removeSessionValue(http_request* s,STORE_TYPES store,char* name);

int 		checkUserRegistered(http_request* s);
char 		setUserRegistered(http_request* s,char status);


int 		dumpSession( http_request *s );
void 		dumpSessions( http_request *s );
unsigned long 	dumpSessionsSize( int *count );


#ifdef __cplusplus
}
#endif

#endif
