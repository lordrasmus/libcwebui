/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/




#include <vld.h> 

/*#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h> 
*/

#include <stdio.h>
#include <winsock2.h>
#include <strsafe.h>
#include <windows.h>
#include <process.h>

#include "webserver.h"



/**************************************************************
*                                                             *
*                   Speicher Verwaltung                       *
*                                                             *
**************************************************************/

void*	PlatformMalloc(SIZE_TYPE size){
	void* ret = malloc(size);
    return ret;
}
void	PlatformFree( void* mem ){
    free(mem);
}



/*********************************************************************

					System informationen

*********************************************************************/





#include "time.h"
TIME_TYPE  PlatformGetTick(void){
	/*SYSTEMTIME systemTime;
	FILETIME fileTime;
	ULARGE_INTEGER uli;
	ULONGLONG systemTimeIn_ms;*/

	time_t seconds;

	seconds = time (NULL);

	return (unsigned long)seconds;

	// GetSystemTimeAsFileTime()

	/*GetSystemTime( &systemTime );
	SystemTimeToFileTime( &systemTime, &fileTime );

	
	
	uli.LowPart = fileTime.dwLowDateTime; // could use memcpy here!
	uli.HighPart = fileTime.dwHighDateTime;

	systemTimeIn_ms = ( uli.QuadPart/10000 );
	while(systemTimeIn_ms > ULONG_MAX)
		systemTimeIn_ms -= ULONG_MAX;
	return (unsigned long)systemTimeIn_ms;*/
}

unsigned long	PlatformGetTicksPerSeconde(void){
	return 1;
}


#ifdef WEBSERVER_USE_SESSIONS
static 	unsigned int guid;
void 	PlatformGetGUID( char* buf,SIZE_TYPE length){	
	int l=0;
	l = sprintf_s(buf,length,"\"Test %d",guid++);
	for(;l<length;l++){
		buf[l]='+';
	}
	buf[length-5]='1';
	buf[length-4]='2';
	buf[length-3]='3';
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
	m->handle = CreateMutex(0, FALSE, 0);
	m->locked = 0;
	if (m->handle == NULL) 
    {
        printf("CreateMutex error: %d\n", GetLastError());
        return -1;
    }
	return 0;
}

int PlatformLockMutex(WS_MUTEX* m){
	DWORD ret = WaitForSingleObject(m->handle, INFINITE);
	m->locked++;
	if(m->locked > 1)
		printf("PlatformLockMutex doppel lock\n");
	if( ret == WAIT_FAILED){
		printf("Lock failed %p\n",(void*)m);
		return -1;
	}else{
		//printf("Locked   %X\n",m);
	}
	return 0;	
}

int PlatformUnlockMutex(WS_MUTEX* m){
	//printf("Unlocked %X\n",m);
	m->locked--;	
	return (ReleaseMutex(m->handle)==0);
}

int PlatformDestroyMutex(WS_MUTEX* m){
	if(m->locked != 0)
		printf("PlatformDestroyMutex ist noch gelocked\n");
	return CloseHandle(m->handle);
}

int PlatformCreateSem(WS_SEMAPHORE_TYPE* sem, int init_value){
	*sem = CreateSemaphore(
		NULL,           // default security attributes
		init_value,  // initial count
		1000,  // maximum count
		NULL);

	return 0;
}
int PlatformWaitSem(WS_SEMAPHORE_TYPE* sem) {
	WaitForSingleObject(
		*sem,   // handle to semaphore
		0L);           // zero-second time-out interval

	return 0;
}
int PlatformPostSem(WS_SEMAPHORE_TYPE* sem) {
	if (!ReleaseSemaphore(
		*sem,  // handle to semaphore
		1,            // increase count by one
		NULL))       // not interested in previous count
	{
		printf("ReleaseSemaphore error: %d\n", GetLastError());
		return -1;
	}

	return 0;
}

int strcasecmp(const char *s1, const char *s2) {
	return _stricmp( s1, s2 );
}
