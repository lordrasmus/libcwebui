/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_API_FUNCTIONS_DEPRICATED_
#define _WEBSERVER_API_FUNCTIONS_DEPRICATED_

//#define ENABLE_DEPRICATED

#ifdef ENABLE_DEPRICATED
	#define FUNC_DEPRICATED( x ) __attribute__((deprecated( x )))
#else
	#define FUNC_DEPRICATED( x )
#endif


void WebserverGenerateGUID(char* buf, int length)	FUNC_DEPRICATED( "use ws_generate_guid") ;

void setRenderVar   (dummy_handler* s,char* name,char* text) FUNC_DEPRICATED( "use ws_set_render_var" ) ;
dummy_var* getRenderVar(dummy_handler* s, const char* name,WS_VAR_FLAGS flags)  FUNC_DEPRICATED( "use ws_get_render_var" ) ;

void WebserverAddFileDir(const char* alias,const char* dir) FUNC_DEPRICATED( "use ws_add_dir" );
void WebserverAddFileDirNoCache(const char* alias,const char* dir) FUNC_DEPRICATED( "use ws_add_dir" );

#endif
