/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_SYSTEM_FILE_ACCESS_H_
#define _WEBSERVER_SYSTEM_FILE_ACCESS_H_

#include "webserver.h"

#ifdef WEBSERVER_USE_WNFS

	extern int wnfs_socket;
#endif


#include "intern/system_file_access_binary.h"
#include "intern/system_file_access_fs.h"
#include "intern/system_file_access_utils.h"
#include "intern/system_file_access_wnfs.h"




#endif
