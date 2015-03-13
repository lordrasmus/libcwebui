/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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


void init_file_access( void );
void free_file_access( void );


int prepare_file_content(WebserverFileInfo* file);
void release_file_content(WebserverFileInfo* file);


#ifndef DISABLE_OLD_TEMPLATE_SYSTEM

	void addTemplateFilePostfix(const char* postfix);
	void addTemplateIgnoreFilePostfix(const char* postfix);

	char isTemplateFile(const char* file);

#endif


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

bool	initFilesystem( void );

global_vars* initWebserver(void);

void shutdownWebserverHandler(void);
void shutdownWebserver(void);



#ifdef __cplusplus
}
#endif

#endif

