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
void addEventSocketWritePersist(socket_info* sock);
void addEventSocketReadWritePersist(socket_info* sock);

/* Horcht nur auf den Verbindungsabbau der Gegenstelle, nicht auf Daten.
 * Nur das epoll-Backend kann das (EPOLLRDHUP); bei select und kqueue
 * verhaelt sich der Aufruf wie delEventSocketAll. */
void addEventSocketPeerClosePersist(socket_info* sock);

void delEventSocketWritePersist(socket_info* sock);
void delEventSocketReadPersist(socket_info* sock);
void delEventSocketAll(socket_info* sock);


/* Must be called before a socket_info is released.
 *
 * epoll and kqueue hand out a snapshot of ready events that carries a raw
 * pointer per entry. A handler that releases a *different* socket - the proxy
 * releases its backend socket together with the client - leaves the entries
 * still waiting in that snapshot pointing at freed memory. Deregistering the
 * events does not help: the kernel cannot take back what it has already
 * delivered.
 *
 * The select backend does not need this and implements it empty; it does not
 * dispatch from a snapshot but reads its own registry once per iteration. */
void eventSocketFreed(socket_info* sock);

void activateEventSocketWrite(socket_info* sock);
void updateWebsocketEventFlags(socket_info* sock, int list_empty);
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
