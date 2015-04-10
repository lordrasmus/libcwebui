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

#define _SCL_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS


#ifdef _MSC_VER
	//#define FD_SETSIZE 512 
	#pragma once
	#include "targetver.h"
	#include <tchar.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
	//#pragma warning(disable:4267) // warning C4267: '=': Konvertierung von 'size_t' nach 'int', Datenverlust mglich
	//#pragma warning(disable:4244) // warning C4244: 'return': Konvertierung von '__w64 int' in 'int', mglicher Datenverlust
	//#pragma warning(disable:4047) // warning C4047: 'Initialisierung': Anzahl der Dereferenzierungen bei 'char *' und 'char (*__w64 )[500]' unterschiedlich	
	#pragma warning(disable:4100) // warning C4100: Unreferenzierter formaler Parameter
	#pragma warning(disable:4127) // warning C4127: Bedingter Ausdruck ist konstant	

#endif


#ifdef _MSC_VER
	#include <winsock2.h>
	#include <strsafe.h>
	//#include <unistd.h>

	#include "webserver.h"
	/**/
	
	/*#include "../platform/include/platform.h"
	#include "system.h"
	
	#include "cookie.h"
	#include "utils.h"
	#include "engine.h"
	#include "webserver_api_functions.h"
	#include "builtinFunctions.h"
	#include "header.h"
	#include "utils.h"
	#include "list.h"
	#include "webserver_log.h"
	#include "variable_store.h"
	#include "variable_render.h"
	
	#include "websockets.h"
	#include "system_sockets.h"
	#include "system_sockets_events.h"

	#ifdef WEBSERVER_USE_SSL
		#include "webserver_ssl.h"
	#endif*/
#endif



/*
 TODO: Hier auf zustzliche Header, die das Programm erfordert, verweisen.
*/
