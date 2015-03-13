/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_SYSTEM_FILE_ACCESS_WNFS_H_
#define _WEBSERVER_SYSTEM_FILE_ACCESS_WNFS_H_

WebserverFileInfo* wnfs_get_file( const char* name );

void wnfs_store_file( WebserverFileInfo* file );

void wnfs_free_file( WebserverFileInfo* file );


#endif
