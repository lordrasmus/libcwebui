/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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

WebserverFileInfo *create_empty_file(int pSize);
void free_empty_file(WebserverFileInfo *file);


#endif
