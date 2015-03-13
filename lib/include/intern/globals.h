/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_GLOBALS_H_
#define _WEBSERVER_GLOBALS_H_


#include "webserver.h"

typedef struct
{
    char server_name[100];
}server_infos;

typedef struct
{
    unsigned char init_called;
    WebserverConfig config;
    server_infos    infos;
}global_vars;


extern global_vars globals;


#endif
