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


#ifndef _WEBSERVERSYSTEM_MEMORY_H_
#define _WEBSERVERSYSTEM_MEMORY_H_


#include "webserver.h"


/********************************************************************
*																	*
*					Speicher Managment								*
*																	*
********************************************************************/

#ifdef _MSC_VER
	#define WebserverMalloc(a) real_WebserverMalloc(a,__FUNCTION__,__FILE__, __LINE__)
#endif
#ifdef __GNUC__

	#ifdef _WEBSERVER_MEMORY_DEBUG_
		#define WebserverMalloc(a) real_WebserverMalloc(a,__FUNCTION__,(char*)__BASE_FILE__, __LINE__)
	#else
		#define WebserverMalloc(a) real_WebserverMalloc(a)
	#endif

	#ifdef BBS_COMPILER
		#define DEBUG_LOG_CHANNEL "Webserver"
		#include <Utils/bbs_common_log.h>
		#include <Utils/bbs_compiler_helper.h>
		#define __ws_assume_aligned __bbs_assume_aligned
	#else
		#if (__GNUC__ == 3 && __GNUC_MINOR__ > 6 )
			#define __ws_assume_aligned __builtin_assume_aligned
		#else
			#define __ws_assume_aligned( a, b ) a
		#endif
	#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

ALL_EXTRA void	initMemoryManager(void) ALL_ATTR;

void    freeMemoryManager(void);
#ifdef _WEBSERVER_MEMORY_DEBUG_
void*	real_WebserverMalloc( const unsigned long size, const char* function, const char* filename, int fileline);
void	printBlocks(void);
#else
void*   real_WebserverMalloc( const unsigned long size );
#endif
void	WebserverFree( void* mem );


unsigned long WebserverMallocedSize(void* mem);
unsigned long WebserverMallocedRealSize(void* mem);
void* WebserverRealloc(void *mem, const unsigned long size );


Parameter* WebserverMallocParameter(void);
void WebserverFreeParameter(Parameter* para);

socket_info* WebserverMallocSocketInfo(void);
void WebserverFreeSocketInfo(socket_info* sock);

Cookie* WebserverMallocCookie(void);
void WebserverFreeCookie(Cookie* cookie);

sessionStore* WebserverMallocSessionStore(void);
void WebserverFreeSessionStore(sessionStore* store);

FUNCTION_PARAS* WebserverMallocFunctionParas(void);
void WebserverFreeFunctionParas(FUNCTION_PARAS *func);

html_chunk* WebserverMallocHtml_chunk(void);
void WebserverFreeHtml_chunk(html_chunk* chunk);
void WebserverFreeHtml_chunk_list(void* chunk);


ws_variable* WebserverMallocVariable_store(void);
void WebserverFreeVariable_store(ws_variable* store);

HttpRequestHeader* WebserverMallocHttpRequestHeader(void);
void WebserverResetHttpRequestHeader(HttpRequestHeader *header);
void WebserverFreeHttpRequestHeader(HttpRequestHeader* header);


#ifdef __cplusplus
}
#endif

#endif

