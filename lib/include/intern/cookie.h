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


#ifndef _COOKIE_H_
#define _COOKIE_H_

#ifdef __GNUC__
	//#include "../platform/include/platform.h"
	#include "dataTypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int checkCookie(char *name,char *value,HttpRequestHeader *header);

void copyCookieValue(char *line,HttpRequestHeader *header,int pos,int pos2);

void parseCookies( char *line,int length,HttpRequestHeader *header);

#ifdef __cplusplus
}
#endif

#endif
