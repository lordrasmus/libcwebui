/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <errno.h>

#ifdef WEBSERVER_USE_SSL
#include <openssl/rand.h>
#endif

#include "webserver.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

/* http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 http://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
*/


/**************************************************************
*                                                             *
*                   Speicher Verwaltung                       *
*                                                             *
**************************************************************/
/*#define MALLOC_DEBUG*/
#ifdef MALLOC_DEBUG
static int mallocs;
#endif
void*	PlatformMalloc ( SIZE_TYPE size ) {
#ifdef MALLOC_DEBUG
    unsigned char *p;
	/*printf ( "Malloc 1 %X (%d) %d\r\n",0,size,mallocs );
	malloc_stats(NULL);*/
    p = malloc ( size+2 );
    p[0]=size;
    p[1]=size>>8;
    printf ( "Malloc  %X (%d) %d\r\n",p+2,size,++mallocs );
    return p+2;
#else
    return malloc ( size );
#endif
}

void	PlatformFree ( void *mem ) {
#ifdef MALLOC_DEBUG
    unsigned char *p=mem-2;
	//malloc_stats(NULL);
    int l = p[0] + ( p[1]<<8 );
    printf ( "Free %X (%d) %d\r\n",mem,l,mallocs-- );
    free ( p );
#else
    free ( mem );
#endif
}

//void	WebserverPrintMemInfo ( void ) {}			/* gibt auf dem DSTni verfuegbaren Arbeitspeicher aus */



/*********************************************************************

					System informationen

*********************************************************************/



TIME_TYPE PlatformGetTick ( void ) {
	#if 0
	struct sysinfo s_info;
	int error;

	error = sysinfo(&s_info);
	if(error != 0){
		printf("code error = %d\n", error);
	}
	return s_info.uptime;
	#endif
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
	#if 0
	pthread_mutexattr_t att;

	if ( m == 0 ){
		printf("PlatformLockMutex wurde nicht initialisiert\n");
		return EINVAL;
	}

	pthread_mutexattr_init(&att);
/*	pthread_mutexattr_settype(&att,PTHREAD_MUTEX_ERRORCHECK_NP); */
	m->locked = 0;
	return pthread_mutex_init(&m->handle,&att);
	#endif
	return 0;
}

int PlatformLockMutex(WS_MUTEX* m){
	#if 0
	/*printf("Locking %X\n",m);*/
	int ret;	
	if ( m == 0 ){
		printf("PlatformLockMutex wurde nicht initialisiert\n");
		return EINVAL;
	}
	ret =  pthread_mutex_lock(&m->handle);
	m->locked++;
	if ( m->locked > 1){
		printf("PlatformLockMutex wurde doppelt gelocked\n");
	}
	return ret;
	#endif
	return 0;
}

int PlatformUnlockMutex(WS_MUTEX* m){
	#if 0
	/*printf("Unlocking %X\n",m);*/
	if ( m == 0 ){
		printf("PlatformLockMutex wurde nicht initialisiert\n");
		return EINVAL;
	}

	m->locked--;
	return pthread_mutex_unlock( &m->handle );
	#endif
	return 0;
}

int PlatformDestroyMutex(WS_MUTEX* m){
	#if 0
	/*printf("Destroy Mutex\n");*/
	if ( m == 0 ){
		return EINVAL;
	}
	if ( m->locked != 0){
		printf("PlatformDestroyMutex Mutex ist gelocked\n");
	}
	return pthread_mutex_destroy(&m->handle);
	#endif
	return 0;
}



int PlatformCreateSem(WS_SEMAPHORE_TYPE* sem, int init_value){
	return 0;
}

int PlatformPostSem(WS_SEMAPHORE_TYPE* sem) {
	return 0;
}

int PlatformWaitSem(WS_SEMAPHORE_TYPE* sem) {
	return 0;
}


int		PlatformClose(int socket){
	//close( socket );
	return 0;
}

