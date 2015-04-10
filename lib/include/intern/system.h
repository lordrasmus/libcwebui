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


#ifndef _WEBSERVERSYSTEM_H_
#define _WEBSERVERSYSTEM_H_

#include "platform.h"
#include "system_memory.h"


#ifndef WebserverUseNFS
	#ifndef WEBSERVER_USE_LOCAL_FILE_SYSTEM
		#ifndef	WEBSERVER_USE_BINARY_FORMAT
			#error "Kein File Mode Konfiguriert ( WebserverConfig.h )"
		#endif
	#endif
#endif

#ifndef WEBSERVER_USE_SSL
	#ifdef WEBSERVER_ONLY_SSL_COOKIES
		#error "Do not use WEBSERVER_ONLY_SSL_COOKIES without WEBSERVER_USE_SSL"
	#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

void setFileDir(const char* alias,const char* dir);
void setFileDirNoCache(const char* alias, const char* dir);

void addTemplateFilePostfix(const char* postfix);
void addTemplateIgnoreFilePostfix(const char* postfix);
char isTemplateFile(const char* file);



void	startWebServer( void );
void	int_startWebServer( void );					/* in system.c enthalten */
void	int_webserverTask(socket_info* request );			/* in system.c enthalten */
bool	startWebserverTask( socket_info* request );

void  	printParameterInfo(HttpRequestHeader *header);



/********************************************************************
*																	*
*					Datei Operationen     							*
*																	*
********************************************************************/

bool	WebServerloadData( void );

global_vars* initWebserver(void);

void shutdownWebserverHandler(void);
void shutdownWebserver(void);



#ifdef __cplusplus
}
#endif

#endif

