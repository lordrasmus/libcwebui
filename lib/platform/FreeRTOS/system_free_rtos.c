/*

 libCWebUI
 Copyright (C) 2007 - 2016  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

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



