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


#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_



#ifndef bool
	#define bool unsigned char
	#define true 	1
	#define false	0
#endif

#ifdef WIN32
	#include "../win/platform_includes.h"
#endif


#if defined( LINUX ) || defined ( OSX )
	#include "../linux/platform_includes.h"
#endif

#ifdef BSD
#define WebServerPrintf printf
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#ifdef DSTni
#include "config.h"
#include "mt.h"
#include "dsttypes.h"
#define WebServerPrintf ConsolePrintf
#endif

#ifdef NET_OS
#define WebServerPrintf printf
#endif


#ifndef WDT_RESET
	#define WDT_RESET
#endif


typedef struct {
	char locked;
	WS_MUTEX_TYPE handle;
}WS_MUTEX;


#endif

