/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_SYSTEM_FILE_ACCESS_FS_H_
#define _WEBSERVER_SYSTEM_FILE_ACCESS_FS_H_

void init_local_file_system( void );
void free_local_file_system( void );

void add_local_file_system_dir(const char* alias,const char* dir, const int use_cache, const int auth_only );

int local_file_system_read_content( WebserverFileInfo *file );
int local_file_system_check_file_modified( WebserverFileInfo *file );

WebserverFileInfo *getFileLocalFileSystem( const char *url_name);


#endif
