

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

#ifndef _WEBSERVER_SYSTEM_FILE_ACCESS_UTILS_H_
#define _WEBSERVER_SYSTEM_FILE_ACCESS_UTILS_H_

void init_file_access_utils(void) ;

int check_blocked_urls( const char* url );


int doNotRamCacheFile( WebserverFileInfo *file );

void copyFilePath(WebserverFileInfo* file, const char* name);
void copyURL(WebserverFileInfo* file, const char* url);

void setFileType(WebserverFileInfo* file);
void generateEtag( WebserverFileInfo *file );


#endif
