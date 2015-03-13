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



#ifdef __cplusplus
}
#endif




#endif
