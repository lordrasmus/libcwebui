/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifndef _SYSTEM_SOCKETS_CONTAINER_H_
#define _SYSTEM_SOCKETS_CONTAINER_H_


#ifdef __cplusplus
extern "C" {
#endif

	void initSocketContainer(void);
	void freeSocketContainer(void);
	void deleteSocket(socket_info* sock);
	void deleteAllSockets(void);
	void addSocketContainer(socket_info* sock);
	void addSocketByGUID(socket_info* sock);
	socket_info* getSocketByGUID(const char* guid);

#ifdef __cplusplus
}
#endif


#endif

