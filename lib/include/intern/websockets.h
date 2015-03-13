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


#ifndef _WEBSOCKETS_H_
#define _WEBSOCKETS_H_

#include "webserver.h"

#define SHORT_WS_FRAMES_T 1

#ifdef SHORT_WS_FRAMES_T
	// 64 Bit Laenge muss auf arm auf 8 byte aligned sein
	// darum websocket frames auf 32 bit begrenzen
	#define WEBSOCK_LEN_T uint32_t
	#define WEBSOCK_LEN_ALLIGN
#else
	#define WEBSOCK_LEN_T uint64_t
	#ifdef _ARM_EABI_
		#define WEBSOCK_LEN_ALLIGN  __attribute__ ((aligned (8)))
	#else
		#define WEBSOCK_LEN_ALLIGN
	#endif
#endif

typedef struct {
	WEBSOCKET_SIGNALS signal;
	char* guid;
	char* url;
	unsigned char* msg;
	WEBSOCK_LEN_T len;
}websocket_queue_msg WEBSOCK_LEN_ALLIGN ;

#ifdef __cplusplus
extern "C" {
#endif



void initWebsocketApi(void);

websocket_queue_msg* create_websocket_input_queue_msg(WEBSOCKET_SIGNALS signal, const char* guid, const char* url, const WEBSOCK_LEN_T msg_length);
websocket_queue_msg* create_websocket_output_queue_msg(WEBSOCKET_SIGNALS signal, const char* guid, const WEBSOCK_LEN_T msg_length);

void insert_websocket_input_queue(websocket_queue_msg* msg);
void insert_websocket_output_queue(websocket_queue_msg* msg);

int sendWebsocketTextFrame(const char* guid, const char* in, const WEBSOCK_LEN_T length);
int sendWebsocketBinaryFrame(const char* guid, const unsigned char* in, const WEBSOCK_LEN_T length);


int checkIskWebsocketConnection(socket_info* sock,HttpRequestHeader* header);
int startWebsocketConnection(socket_info* sock);
void calcWebsocketSecKeys(socket_info* request);

char* getWebsocketStoreGUID(char* guid);

int sendWebsocketFrameReal(const char* guid, const unsigned char* in, const WEBSOCK_LEN_T length);
int recFrame(socket_info *sock);
int closeWebsocket(char* guid);
void initWebsocketStructures(socket_info* sock);

void sendCloseFrame(socket_info *sock);
void sendCloseFrame2(const char* guid);


void websocket_event_handler(socket_info* sock, EVENT_TYPES event_type);

#ifdef __cplusplus
}
#endif

#endif
