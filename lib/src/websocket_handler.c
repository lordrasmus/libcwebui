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

#ifdef WEBSERVER_USE_WEBSOCKETS

static int handleWebsocket(socket_info* sock, EVENT_TYPES type) {
	html_chunk* chunk;
	int send_bytes;
	int to_send;
	SOCKET_SEND_STATUS status;



	switch (type) {

		case EVENT_TIMEOUT:
			LOG( WEBSOCKET_LOG, ERROR_LEVEL, 0, "Event Type EVENT_TIMEOUT not handled", "");
			return -1;
		case EVENT_SIGNAL:
			LOG( WEBSOCKET_LOG, ERROR_LEVEL, 0, "Event Type EVENT_SIGNAL not handled", "");
			return -1;
		case EVENT_PERSIST:
			LOG( WEBSOCKET_LOG, ERROR_LEVEL, 0, "Event Type EVENT_PERSIST not handled", "");
			return -1;

		case EVENT_READ: /* READ */
			/*
			  Read muss nicht gelockt werden da nicht auf die websocket_chunk_list zugegriffen wird
			  ausserdem liest nur der Hauptthread vom socket
			*/
			#if _WEBSERVER_WEBSOCKET_DEBUG_ > 4
				LOG( WEBSOCKET_LOG, ERROR_LEVEL, 0, "Event Type EVENT_READ", "");
			#endif
			if (recFrame(sock) < 0) {
				return -1;
			}
			break;

		case EVENT_WRITE: /* WRITE */

			#if _WEBSERVER_WEBSOCKET_DEBUG_ > 4
				LOG( WEBSOCKET_LOG, ERROR_LEVEL, 0, "Event Type EVENT_WRITE", "");
			#endif

			PlatformLockMutex(&sock->socket_mutex);

			while (1) {

				if (unlikely(ws_list_empty(&sock->websocket_chunk_list) == 1)) break;

				chunk = (html_chunk*) ws_list_get_at(&sock->websocket_chunk_list, 0);

				if (unlikely(chunk == 0)) break;

				to_send = chunk->length - sock->file_infos.file_send_pos;
				status = WebserverSend(sock, (unsigned char*) &chunk->text[sock->file_infos.file_send_pos], to_send, 0, &send_bytes);

				if (likely (send_bytes == to_send) ) {
					sock->file_infos.file_send_pos = 0;
					ws_list_delete(&sock->websocket_chunk_list, chunk);
					WebserverFreeHtml_chunk(chunk);
					if (likely ( status == SOCKET_SEND_NO_MORE_DATA ) )
						continue;
				}

				if ((status == SOCKET_SEND_UNKNOWN_ERROR) || (status == SOCKET_SEND_CLIENT_DISCONNECTED)) {
					PlatformUnlockMutex(&sock->socket_mutex);
					return -1;
				}

				/* Der Buffer konnte nur teilweise gesendet werden */
				if ((status == SOCKET_SEND_NO_MORE_DATA) && (send_bytes < to_send)) {
					sock->file_infos.file_send_pos = send_bytes;
					break;
				}
				
				if ( status == SSL_PROTOCOL_ERROR ){
					PlatformUnlockMutex(&sock->socket_mutex);
					return -1;
				}				
				
				LOG( WEBSOCKET_LOG, ERROR_LEVEL, sock->socket, "Darf hier niemals ankommen  status: %d  send_bytes: %d  to_send: %d", status,send_bytes,to_send);
				PlatformUnlockMutex(&sock->socket_mutex);
				return -1;
			}

			if (ws_list_empty(&sock->websocket_chunk_list) == 1) {
				delEventSocketAll(sock);
				addEventSocketReadPersist(sock);
			}

			PlatformUnlockMutex(&sock->socket_mutex);
			break;
	}

	return 0;
}

void websocket_event_handler(socket_info* sock, EVENT_TYPES event_type) {
	int ret;
	PlatformLockMutex(&sock->socket_mutex);
	ret = ws_list_empty(&sock->websocket_chunk_list);
	PlatformUnlockMutex(&sock->socket_mutex);

	/* Socket nur schliessen wenn alle Daten gesendet wurden */
	if ((sock->closeSocket == 1) && (ret == 1)) {
		WebserverConnectionManagerCloseRequest(sock);
		return;
	}

	ret = handleWebsocket(sock, event_type);

	if (ret < 0) {
		WebserverConnectionManagerCloseRequest(sock);
		return;
	}
}

#endif
