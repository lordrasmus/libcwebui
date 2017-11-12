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
