/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

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

#ifndef WEBSOCKET_INIT_INBUFFER_SIZE
	#define WEBSOCKET_INIT_INBUFFER_SIZE 50
	#warning WEBSOCKET_INIT_INBUFFER_SIZE not defined, defaults to 50
#endif


WS_THREAD websocket_input_thread;
WS_THREAD websocket_output_thread;

static ws_MessageQueue* websocket_input_queue;
static ws_MessageQueue* websocket_output_queue;

/* Pushed by stopWebsocketApi() to end a thread; never freed. Queued behind
 * everything already waiting, so pending messages are handled first. */
static websocket_queue_msg websocket_stop_msg;

/* Set while both threads run. Messages queued after the stop are released
 * right away instead of waiting in a queue nobody reads any more. */
static volatile int websocket_threads_running = 0;

static void free_input_msg(websocket_queue_msg* msg) {
	WebserverFree(msg->guid);
	WebserverFree(msg->msg);
	WebserverFree(msg->url);
	WebserverFree(msg);
}

static void free_output_msg(websocket_queue_msg* msg) {
	WebserverFree(msg->guid);
	WebserverFree(msg->msg);
	WebserverFree(msg);
}

static void websocket_input_thread_function( void ) {
	websocket_queue_msg* msg;
	while (1) {
		msg = (websocket_queue_msg*) ws_popQueue(websocket_input_queue);
		if (msg == &websocket_stop_msg) {
			break;
		}

		handleWebsocketConnection(msg->signal, msg->guid, msg->url, msg->binary, (char*)msg->msg, msg->len);

		free_input_msg(msg);
	}

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
	if (websocket_threads_running == 0) {
		free_input_msg(msg);
		return;
	}
	ws_pushQueue(websocket_input_queue, (void*) msg);
}

void insert_websocket_output_queue(websocket_queue_msg* msg){
	if (websocket_threads_running == 0) {
		free_output_msg(msg);
		return;
	}
	ws_pushQueue(websocket_output_queue, (void*) msg);
}


static void websocket_output_thread_function( void ) {
	websocket_queue_msg* msg;
	while (1) {
		msg = (websocket_queue_msg*) ws_popQueue(websocket_output_queue);
		if (msg == &websocket_stop_msg) {
			break;
		}

		sendWebsocketFrameReal(msg->guid, msg->msg, msg->len);

		free_output_msg(msg);
	}

}

void initWebsocketApi(void) {
	websocket_input_queue = ws_createMessageQueue();
	websocket_output_queue = ws_createMessageQueue();
	websocket_threads_running = 1;
	PlatformCreateThread(&websocket_input_thread, websocket_input_thread_function);
	PlatformCreateThread(&websocket_output_thread, websocket_output_thread_function);
	initWebsocketStreaming();
}

/*
 * End both websocket threads and wait for them, so that nothing they do can
 * overlap with the shutdown releasing sessions, config, handler tables and
 * the socket container. Call it after all connections are closed: the
 * disconnect messages that closing queued are still delivered to the
 * handlers, the stop marker comes behind them.
 *
 * The input thread goes first, its handlers may still queue frames; those
 * are sent (to nobody, the sockets are gone) or dropped by the output thread
 * before it stops. The queues stay allocated: another application thread
 * may still try to send, and finds its message released instead of a freed
 * queue.
 */
void stopWebsocketApi(void) {
	if (websocket_threads_running == 0) {
		return;
	}
	ws_pushQueue(websocket_input_queue, &websocket_stop_msg);
	PlatformJoinThread(&websocket_input_thread);

	ws_pushQueue(websocket_output_queue, &websocket_stop_msg);
	PlatformJoinThread(&websocket_output_thread);

	websocket_threads_running = 0;
}

void initWebsocketStructures(socket_info* sock) {
	sock->websocket_buffer = (unsigned char*) WebserverMalloc( WEBSOCKET_INIT_INBUFFER_SIZE );
	sock->websocket_guid = (char*) WebserverMalloc( WEBSERVER_GUID_LENGTH + 1);

	/* Initialize streaming state */
	sock->websocket_streaming_active = 0;
	sock->websocket_stream_fragmented = 0;
	sock->websocket_stream_remaining = 0;
	sock->websocket_stream_mask_offset = 0;
	sock->websocket_stream_ctx = NULL;
	sock->websocket_stream_handler = NULL;
}

/* RFC 6455 4.1: the key is 16 random bytes in base64, always 24 characters:
 * 22 from the alphabet and "==" */
#define WEBSOCKET_KEY_LENGTH 24

static int validWebsocketKey(const char* key) {
	size_t i;

	if (strlen(key) != WEBSOCKET_KEY_LENGTH) {
		return 0;
	}
	for (i = 0; i < WEBSOCKET_KEY_LENGTH - 2; i++) {
		char c = key[i];
		if (!(((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) ||
		      ((c >= '0') && (c <= '9')) || (c == '+') || (c == '/'))) {
			return 0;
		}
	}
	return (key[22] == '=') && (key[23] == '=');
}

/*
 * Sec-WebSocket-Accept for a client key (RFC 6455 4.2.2): base64 of the
 * SHA-1 of the key followed by the protocol GUID. out takes at least 29
 * bytes. Returns 0, or -1 for a key that is not a valid one.
 */
int websocketAcceptKey(const char* key, unsigned char* out, int out_size) {
	static const char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	char buffer[WEBSOCKET_KEY_LENGTH + sizeof(guid)];
	char sha1[21];

	if ((key == NULL) || !validWebsocketKey(key) || (out_size < 29)) {
		return -1;
	}
	memcpy(buffer, key, WEBSOCKET_KEY_LENGTH);
	memcpy(&buffer[WEBSOCKET_KEY_LENGTH], guid, sizeof(guid));
	WebserverSHA1((unsigned char*) buffer, sizeof(buffer) - 1, (unsigned char*) sha1);
	WebserverBase64Encode((unsigned char*) sha1, 20, out, out_size);
	return 0;
}

int checkIskWebsocketConnection(socket_info* sock) {
	HttpRequestHeader* header;
		
	if (sock == 0) {
		return 0;
	}

	header = sock->header;

	if (header == 0) {
		return 0;
	}

	header->isWebsocket = 0;

	if (header->SecWebSocketKey == 0){
		return 0;
	}

	if (header->Connection == 0) {
		return 0;
	}

	if (header->Upgrade == 0) {
		return 0;
	}

	/* RFC 7230: Connection header is a comma-separated list of tokens.
	 * Check that "Upgrade" is a complete token, not a substring.
	 * Example: "keep-alive, Upgrade" or "Upgrade" */
	{
		int upgrade_found = 0;
		const char* p = header->Connection;
		while (*p) {
			/* Skip leading whitespace */
			while (*p == ' ' || *p == '\t') p++;
			/* Find end of token (comma or end of string) */
			const char* token_start = p;
			while (*p && *p != ',') p++;
			/* Calculate token length, trim trailing whitespace */
			size_t token_len = p - token_start;
			while (token_len > 0 && (token_start[token_len-1] == ' ' || token_start[token_len-1] == '\t')) {
				token_len--;
			}
			/* Check if token matches "Upgrade" (case-insensitive) */
			if (token_len == 7 &&
			    (token_start[0] == 'U' || token_start[0] == 'u') &&
			    (token_start[1] == 'p' || token_start[1] == 'P') &&
			    (token_start[2] == 'g' || token_start[2] == 'G') &&
			    (token_start[3] == 'r' || token_start[3] == 'R') &&
			    (token_start[4] == 'a' || token_start[4] == 'A') &&
			    (token_start[5] == 'd' || token_start[5] == 'D') &&
			    (token_start[6] == 'e' || token_start[6] == 'E')) {
				upgrade_found = 1;
				break;
			}
			/* Skip comma */
			if (*p == ',') p++;
		}
		if (!upgrade_found) {
			return 0;
		}
	}

	/* RFC 6455: Upgrade header value is case-insensitive */
	if (0 == strcasecmp(header->Upgrade, "websocket")) {

		if ( header->SecWebSocketVersion < 13 ){
			/* RFC 6455 Section 4.4: Send supported version(s) in response */
			printHeaderChunk(sock,"HTTP/1.1 426 Upgrade Required\r\n");
			header->response_status = 426;
			printHeaderChunk(sock,"Sec-WebSocket-Version: 13\r\n");
			printHeaderChunk(sock,"\r\n");
			#ifdef _WEBSERVER_HEADER_DEBUG_
			LOG(HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket,"Websocket Handshake Error: Version ( %d ) is < 13",header->SecWebSocketVersion);
			#endif
			header->isWebsocket = 2; // falsche websocket version
			return 1;
		}

		/* RFC 6455 4.2.1: a key that is not 16 bytes in base64 gets 400.
		 * Checked here, before anything is computed from it: an overlong
		 * key used to be copied without its terminator into a fixed
		 * buffer on the stack and then appended to. */
		if (!validWebsocketKey(header->SecWebSocketKey)) {
			printHeaderChunk(sock,"HTTP/1.1 400 Bad Request\r\n");
			header->response_status = 400;
			printHeaderChunk(sock,"Content-Length: 0\r\n");
			printHeaderChunk(sock,"\r\n");
			header->isWebsocket = 2; /* refused, the response is prepared */
			return 1;
		}

		header->isWebsocket = 1; // websocket ok
		return 1;
	}
	return 0;
}

int startWebsocketConnection(socket_info* sock) {
	http_request *s;
	sock->active = 0;

	/* The key was validated by checkIskWebsocketConnection() */
	if (sock->header->SecWebSocketKey != 0) {
		websocketAcceptKey(sock->header->SecWebSocketKey, sock->header->WebSocketOutHash,
		                   (int)sizeof(sock->header->WebSocketOutHash));
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

	if ( s->store != 0 ){
		sock->websocket_store_guid = WebserverMalloc( WEBSERVER_GUID_LENGTH + 1 );
		memset(sock->websocket_store_guid, 0, WEBSERVER_GUID_LENGTH + 1 );
		memcpy( sock->websocket_store_guid, s->store->guid, WEBSERVER_GUID_LENGTH );
		//printf("copy Store GUID : %s\n", sock->websocket_store_guid);
	}

	if (handleWebsocketConnection(WEBSOCKET_SIGNAL_CONNECT, sock->websocket_guid, sock->header->url, 0 ,0 , 0) < 0) {
		printf("\nWebsocket connection error\n\n");
		sendCloseFrame(sock);
		sock->closeSocket = 1;
	}

	return 0;
}




char* getWebsocketStoreGUID(char* guid) {
	socket_info *sock = getSocketByGUID(guid); /* Locked */
	http_request *s;
	char* ret = 0;
	if (sock == 0){
		return 0;
	}
	if (sock->closeSocket == 1){
		PlatformUnlockMutex(&sock->socket_mutex);
		return 0;
#ifdef ENABLE_DEVEL_WARNINGS
		#warning mal tracen ( unlock ist neu )
#endif
	}

	if (sock->s != 0) {
		s = (http_request*) sock->s;
		if (s->store != 0){
			ret = s->store->guid;
		}
	}

	PlatformUnlockMutex(&sock->socket_mutex);

	return ret;
}

long getWebsocketStoreTimeout ( char* guid ){
	socket_info *sock = getSocketByGUID(guid);
	if (sock == 0){
		return -1;
	}

	if (sock->closeSocket == 1) {
		PlatformUnlockMutex(&sock->socket_mutex);
		return -1;
	}

	char store_guid[ WEBSERVER_GUID_LENGTH + 1 ];
	strncpy( store_guid , sock->websocket_store_guid,WEBSERVER_GUID_LENGTH );
	store_guid[WEBSERVER_GUID_LENGTH] = '\0';

	//printf("Store GUID : %s WS : %s\n", store_guid, guid );

	PlatformUnlockMutex(&sock->socket_mutex);

	long tmp = getSessionTimeoutByGUID( store_guid , SESSION_STORE );
	if ( tmp < 0 ){
		return -1;
	}

	long session_timeout = getConfigInt("session_timeout");
	/* Check before subtraction to avoid potential overflow */
	if ( tmp > session_timeout ){
		return -1;
	}
	long timeout = session_timeout - tmp;

	/*http_request *s = sock->s;
	if ( s == 0 ) return ULONG_MAX;

	sessionStore *ss = (sessionStore*)s->store;
	if ( ss == 0 ) return ULONG_MAX;

	unsigned long diff = PlatformGetTick() - ss->last_use;



	unsigned long timeout = getConfigInt("session_timeout") - diff;*/



	return timeout;
}

int closeWebsocket(char* guid) {
	socket_info *sock = getSocketByGUID(guid); /* Locked */
	if (sock == 0){
		return 0;
	}
	if (sock != 0){
		sock->closeSocket = 1;
	}

	PlatformUnlockMutex(&sock->socket_mutex);
#ifdef ENABLE_DEVEL_WARNINGS
	#warning mal tracen ( unlock ist neu )
#endif

	return 0;
}

#endif

