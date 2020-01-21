/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



#ifndef _SYSTEM_SOCKETS_EVENT_H_
#define _SYSTEM_SOCKETS_EVENT_H_



#include "webserver.h"

typedef enum{
	UNDEFINED,
	NO_MORE_DATA,
	DATA_PENDING,
	CLIENT_DICONNECTED
}CLIENT_WRITE_DATA_STATUS;


#ifdef __cplusplus
extern "C" {
#endif

void eventHandler(int a,short b, void *t);

void handleer(int a,short b, void *t);

void addEventSocketRead(socket_info* sock);
void addEventSocketReadPersist(socket_info* sock);
void addEventSocketWrite(socket_info* sock);
void addEventSocketWritePersist(socket_info* sock);
void addEventSocketReadWritePersist(socket_info* sock);

void delEventSocketWritePersist(socket_info* sock);
void delEventSocketReadPersist(socket_info* sock);
void delEventSocketAll(socket_info* sock);

void commitSslEventFlags( socket_info* sock );

void deleteEvent(socket_info* sock);
char checkEventSocket(socket_info* sock);
char waitEvents(void);
void initEvents(void);
void freeEvents(void);
void breakEvents(void);

void setEventSocketWrite(socket_info* info);
void setEventSocketRead(socket_info* info);

CLIENT_WRITE_DATA_STATUS handleClientWriteData(socket_info* sock);
CLIENT_WRITE_DATA_STATUS handleClientWriteDataNotCachedReadWrite(socket_info* sock, socket_file_infos* file);



#ifdef __cplusplus
}
#endif




#endif
