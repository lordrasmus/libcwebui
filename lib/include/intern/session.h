/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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


#ifdef __cplusplus
extern "C" {
#endif

void 		createSession(http_request* s,unsigned char ssl_store);

void 		initSessions(void);
void 		freeSessions(void);

void 		checkSessionCookie(http_request* s);
void        restoreSession(http_request* s,int lock_stores, int create_session );


long 		getSessionTimeoutByGUID(char* store_guid, STORE_TYPES store );
char 		setSessionValueByGUID(char* store_guid,STORE_TYPES store,const char* name,const char* value);
char 		setSessionValue(http_request* s,STORE_TYPES store,const char* name,const char* value);
ws_variable*  	getSessionValueByGUID(char* store_guid,STORE_TYPES store,const char* name);
ws_variable*  	getSessionValue(http_request* s,STORE_TYPES store,const char* name);
ws_variable* addSessionValue(http_request* s, STORE_TYPES store, const char* name);
void 		printSessionValue(http_request* s,STORE_TYPES store,char* name);
char 		removeSessionValue(http_request* s,STORE_TYPES store,char* name);

int 		checkUserRegistered(http_request* s);
char 		setUserRegistered(http_request* s,char status);


int 		dumpSessionStore( http_request *s );
void 		dumpSessions( http_request *s );
unsigned long 	dumpSessionsSize( int *count );


#ifdef __cplusplus
}
#endif

#endif
