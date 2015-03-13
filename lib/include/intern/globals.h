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

#ifndef _WEBSERVER_GLOBALS_H_
#define _WEBSERVER_GLOBALS_H_


#include "webserver.h"

typedef struct
{
    //char server_ip[100];
    //char server_netmask[100];
    //char server_mac[100];
    char server_name[100];
}server_infos;

typedef struct
{
    unsigned char init_called;
    WebserverConfig config;
    server_infos    infos;
    //ws_variable_store*	var_store;
}global_vars;


extern global_vars globals;


#endif
