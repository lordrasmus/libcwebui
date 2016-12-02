/*

 libCWebUI
 Copyright (C) 2007 - 2016  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

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

#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif



/*
 Protokoll

 http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-16


 http://www.cs.uwaterloo.ca/~brecht/servers/openfiles.html
 http://www.hpl.hp.com/techreports/2000/HPL-2000-174.pdf
 http://www.kegel.com/c10k.html#top
 http://www.acme.com/software/thttpd/
 http://redmine.lighttpd.net/wiki/1/Docs:Performance
 http://jwebsocket.org/
*/

#ifdef WEBSERVER_USE_WEBSOCKETS

#ifndef WEBSERVER_USE_SSL
	#error "Webockets nur mit SSL support"
#endif

#ifndef WEBSOCKET_INIT_INBUFFER_SIZE
	#define WEBSOCKET_INIT_INBUFFER_SIZE 50
	#warning WEBSOCKET_INIT_INBUFFER_SIZE not defined, defaults to 50
#endif


pthread_t websocket_input_thread;
pthread_t websocket_output_thread;

static ws_MessageQueue* websocket_input_queue;
static ws_MessageQueue* websocket_output_queue;

void *websocket_input_thread_function( UNUSED_PARA void *ptr) {
	websocket_queue_msg* msg;
	while (1) {
		msg = (websocket_queue_msg*) ws_popQueue(websocket_input_queue);

		handleWebsocketConnection(msg->signal, msg->guid, msg->url,1 , (char*)msg->msg, msg->len);

		WebserverFree(msg->guid);
		WebserverFree(msg->msg);
		WebserverFree(msg->url);
		WebserverFree(msg);
	}
	return 0;
}



websocket_queue_msg* create_websocket_input_queue_msg(WEBSOCKET_SIGNALS signal, const char* guid, const char* url, const WEBSOCK_LEN_T msg_length) {
	websocket_queue_msg* tmp = (websocket_queue_msg*) WebserverMalloc( sizeof(websocket_queue_msg) );

	tmp->signal = signal;

	tmp->msg = (unsigned char*) WebserverMalloc( msg_length );
	tmp->len = msg_length;

	tmp->guid = (char*) WebserverMalloc( strlen(guid) +1 );
	strcpy(tmp->guid, guid);

	tmp->url = (char*) WebserverMalloc( strlen( url) + 1 );
	strcpy(tmp->url, url);
	return tmp;
}

websocket_queue_msg* create_websocket_output_queue_msg(WEBSOCKET_SIGNALS signal, const char* guid, const WEBSOCK_LEN_T msg_length) {
	websocket_queue_msg* msg = (websocket_queue_msg*) WebserverMalloc( sizeof(websocket_queue_msg) );

	msg->signal = signal;

	msg->guid = (char*) WebserverMalloc( strlen(guid) + 1 );
	strcpy(msg->guid, guid);

	msg->msg = (unsigned char*) WebserverMalloc( msg_length );
	msg->len = msg_length;

	return msg;
}


void insert_websocket_input_queue(websocket_queue_msg* msg){
	ws_pushQueue(websocket_input_queue, (void*) msg);
}

void insert_websocket_output_queue(websocket_queue_msg* msg){
	ws_pushQueue(websocket_output_queue, (void*) msg);
}


void *websocket_output_thread_function( UNUSED_PARA void *ptr) {
	websocket_queue_msg* msg;
	while (1) {
		msg = (websocket_queue_msg*) ws_popQueue(websocket_output_queue);

		sendWebsocketFrameReal(msg->guid, msg->msg, msg->len);

		WebserverFree(msg->guid);
		WebserverFree(msg->msg);
		WebserverFree(msg);
	}
	return 0;
}

void initWebsocketApi(void) {
	websocket_input_queue = ws_createMessageQueue();
	websocket_output_queue = ws_createMessageQueue();
	pthread_create(&websocket_input_thread, NULL, websocket_input_thread_function, 0);
	pthread_create(&websocket_output_thread, NULL, websocket_output_thread_function, 0);

}

void initWebsocketStructures(socket_info* sock) {
	sock->websocket_buffer = (unsigned char*) WebserverMalloc( WEBSOCKET_INIT_INBUFFER_SIZE );
	sock->websocket_guid = (char*) WebserverMalloc( WEBSERVER_GUID_LENGTH + 1);

}

int checkIskWebsocketConnection(socket_info* sock,HttpRequestHeader* header) {
	if (header == 0) {
		return -1;
	}

	if (header->Connection != 0) {
		if (0 != strstr(header->Connection, "Upgrade")) { /* Firefox sendet keep-alive, Upgrade */
			if (header->Upgrade != 0) {
				if ((0 == strcmp(header->Upgrade, "websocket")) || (0 == strcmp(header->Upgrade, "WebSocket")) || (0 == strcmp(header->Upgrade, "Websocket"))  ) {
					header->isWebsocket = 1;

					if ( header->SecWebSocketVersion < 13 ){
						printHeaderChunk(sock,"HTTP/1.1 400 Bad Request\r\n");
						printHeaderChunk(sock,"Sec-WebSocket-Version: 13\r\n");
						printHeaderChunk(sock,"\r\n");
						printf("Error Connection Websocket: Version ( %d ) is < 13 \n",header->SecWebSocketVersion);
						header->isWebsocket = 2;
						return 3;
					}

					if (header->SecWebSocketKey != 0) return 2;
					if (header->SecWebSocketKey1 != 0) return 2;

					return 1;
				}
			}
		}
	}
	return -1;
}

int startWebsocketConnection(socket_info* sock) {
	char buffer[170];
	char buffer2[21];
	http_request *s;
	sock->active = 0;

	if (sock->header->SecWebSocketVersion < 7) {
		if (sock->header->SecWebSocketKey1 != 0) {
			calcWebsocketSecKeys(sock);
		}
	} else {
		if (sock->header->SecWebSocketKey != 0) {
			strncpy( buffer, sock->header->SecWebSocketKey, 100 );
			strcat(buffer, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
			WebserverSHA1((unsigned char*) buffer, strlen(buffer), (unsigned char*) buffer2);
			WebserverBase64Encode((unsigned char*) buffer2, 20, sock->header->WebSocketOutHash, 40);
		}
	}

	sendHeaderWebsocket(sock);

	s = (http_request*) WebserverMalloc( sizeof(http_request) );
	memset(s, 0, sizeof(http_request));

	s->socket = sock;
	s->header = sock->header;
	sock->websocket_buffer_offset = 0;
	sock->s = s;

	generateGUID(sock->websocket_guid, WEBSERVER_GUID_LENGTH);
	addSocketByGUID(sock);

	checkSessionCookie(s);
	restoreSession(s,0, 1 );

	if (handleWebsocketConnection(WEBSOCKET_SIGNAL_CONNECT, sock->websocket_guid, sock->header->url, 0 ,0 , 0) < 0) {
		printf("\nConnect fehler\n\n");
		sendCloseFrame(sock);
		sock->closeSocket = 1;
	}

	return 0;
}

static void setChallengeNumber(unsigned char* buf, uint32_t number) {
	int i;
	unsigned char* p = buf + 3;
	for (i = 0; i < 4; i++) {
		*p = number & 0xFF;
		--p;
		number >>= 8;
	}
}

static void generateExpectedChallengeResponse(uint32_t number1, uint32_t number2, char key3[8], char expectedChallenge[16]) {
	unsigned char challenge[16];

	setChallengeNumber(&challenge[0], number1);
	setChallengeNumber(&challenge[4], number2);
	memcpy(&challenge[8], key3, 8);

	WebserverMD5(challenge, 16, (unsigned char*) expectedChallenge);
}

unsigned long calckey(char* buffer) {
	int offset;
	unsigned long key = 0;
	unsigned int spaces = 0;
	int i;
	unsigned long multi = 1;
	char data;
	SIZE_TYPE tmp;

	tmp = strlen(buffer) - 1;
	if ( tmp > INT_MAX ){
		perror("calckey: tmp > INT_MAX");
		offset = INT_MAX;
	}else{
		offset = (int)tmp;
	}

	for ( i = offset; i >= 0; i--) {
		data = buffer[i];
		if ((data >= '0') && (data <= '9')) {
			key += ((unsigned long) (data) - '0') * multi;
			multi *= 10;
		}
		if (data == ' ') {
			spaces++;
		}
	}
	if ( spaces > 0 )
		key /= spaces;
	return key;
}

void calcWebsocketSecKeys(socket_info* request) {
	unsigned int k1, k2;
	unsigned char google[16];

	if (request->header->SecWebSocketKey1 == 0) /* Alte (BETA) Protokoll Version */
		return;

	if (request->header->SecWebSocketKey2 == 0) return;

	k1 = calckey(request->header->SecWebSocketKey1);
	k2 = calckey(request->header->SecWebSocketKey2);

	generateExpectedChallengeResponse(k1, k2, request->header->WebSocketKey3, (char*) google);

	memcpy(request->header->WebSocketOutHash, google, 16);

}


char* getWebsocketStoreGUID(char* guid) {
	socket_info *sock = getSocketByGUID(guid); /* Locked */
	http_request *s;
	char* ret = 0;
	if (sock == 0) return 0;
	if (sock->closeSocket == 1){
		PlatformUnlockMutex(&sock->socket_mutex);
		return 0;
#ifdef ENABLE_DEVEL_WARNINGS
		#warning mal tracen ( unlock ist neu )
#endif
	}

	if (sock->s != 0) {
		s = (http_request*) sock->s;
		if (s->store != 0) ret = s->store->guid;
	}

	PlatformUnlockMutex(&sock->socket_mutex);

	return ret;
}

unsigned long getWebsocketStoreTimeout ( char* guid ){
	socket_info *sock = getSocketByGUID(guid);
	if (sock == 0) return -1;

	if (sock->closeSocket == 1) {
		PlatformUnlockMutex(&sock->socket_mutex);
		return -1;
	}


	//printf("sock->s : %p\n",sock->s);
	http_request *s = sock->s;
	//printf("sock->s->store : %p\n",s->store);

	sessionStore *ss = (sessionStore*)s->store;
	unsigned long last_use = ss->last_use;

	//printf("sock->s->store->last_use : %lu\n",last_use);

	unsigned long diff = PlatformGetTick() - ss->last_use;
	printf("diff : %lu\n",diff);

	PlatformUnlockMutex(&sock->socket_mutex);

	unsigned long timeout = getConfigInt("session_timeout") - diff;
	printf("timeout : %lu\n",timeout);

	return timeout;
}

int closeWebsocket(char* guid) {
	socket_info *sock = getSocketByGUID(guid); /* Locked */
	if (sock == 0) return 0;
	if (sock != 0) sock->closeSocket = 1;

	PlatformUnlockMutex(&sock->socket_mutex);
#ifdef ENABLE_DEVEL_WARNINGS
	#warning mal tracen ( unlock ist neu )
#endif

	return 0;
}

#endif

