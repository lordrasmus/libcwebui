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


#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "platform-defines.h"
#include "platform-sockets.h"
#include "platform-file-access.h"



/********************************************************************
*																	*
*					Speicher Managment								*
*																	*
********************************************************************/

void*	PlatformMalloc(SIZE_TYPE size);
void	PlatformFree( void* mem );

/********************************************************************
*																	*
*					System Informationen							*
*																	*
********************************************************************/

void 			PlatformGetGUID(char* buf,SIZE_TYPE length);
//void			PlatformGetIP(session *s);
void 			PlatformGetIPv6(char* bytes);
//void 			PlatformGetMAC(session *s);
//void 			PlatformGetNetmask(session *s);
TIME_TYPE   	PlatformGetTick(void);
unsigned long	PlatformGetTicksPerSeconde(void);


/********************************************************************
*																	*
*					Thread Lock ( Mutex )							*
*																	*
********************************************************************/


ALL_EXTRA int PlatformCreateMutex(WS_MUTEX* m) ALL_ATTR;
ALL_EXTRA int PlatformLockMutex(WS_MUTEX* m) ALL_ATTR;
ALL_EXTRA int PlatformUnlockMutex(WS_MUTEX* m) ALL_ATTR;
ALL_EXTRA int PlatformDestroyMutex(WS_MUTEX* m) ALL_ATTR;

#endif

