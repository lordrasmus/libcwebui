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


#ifndef _SYSTEM_FILE_CACHE_
#define _SYSTEM_FILE_CACHE_

#include "webserver.h"


void initFileCache(void);
void freeFileCache(void);

void addFileToCache(WebserverFileInfo* wfi);

WebserverFileInfo* getFileFromRBCache(char* name);

void dumpLoadedFiles ( http_request *s );
unsigned long getLoadedFilesSize(int *p_count);

#endif
