/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/




#ifdef WEBSERVER_USE_SSL
#include <openssl/rand.h>
#endif

#include "webserver.h"




void*	PlatformMalloc ( SIZE_TYPE size ) {
	return 0;
}

void	PlatformFree ( void *mem ) {
}

void	WebserverPrintMemInfo ( void ) {}



/*********************************************************************

					System informationen

*********************************************************************/



TIME_TYPE PlatformGetTick ( void ){
	return 0;
}

unsigned long PlatformGetTicksPerSeconde ( void ) {
    return (unsigned long)1; /* Konstante HZ benutzen ?? */
}

#ifdef WEBSERVER_USE_SESSIONS
static 	unsigned int guid;
void 	PlatformGetGUID ( char* buf,SIZE_TYPE length ) {
    SIZE_TYPE l=0;
	int ret;

    if ( buf == 0 ){
    	return;
    }

    ret = snprintf ( buf, length, "\"Test %d", guid++ );
    if ( ret < 0 ){
		buf[0]='\0';
		perror("PlatformGetGUID: snprintf failed\n");
		return;
	}
	l = (SIZE_TYPE)ret;
    for ( ; l < length ; l++ ) {
        buf[l]='+';
    }
    buf[length-2]='\"';
    buf[length-1]='\0';
}
#endif






/********************************************************************
*																	*
*					Thread Lock ( Mutex )							*
*																	*
********************************************************************/

int PlatformCreateMutex(WS_MUTEX* m){
	return 0;
}

int PlatformLockMutex(WS_MUTEX* m){
	return 0;
}

int PlatformUnlockMutex(WS_MUTEX* m){
	return 0;
}

int PlatformDestroyMutex(WS_MUTEX* m){
	return 0;
}



