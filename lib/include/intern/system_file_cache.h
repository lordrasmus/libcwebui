/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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
