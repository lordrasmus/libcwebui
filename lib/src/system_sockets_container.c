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

#include "red_black_tree.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

/* Listeners, accepted clients and proxy backends (sockets injected with
 * WebserverInjectExternFD() are not part of it).
 * Intrusive doubly linked list through socket_info::sock_list_prev/next,
 * so deleteSocket() unlinks in O(1) instead of searching the whole list,
 * and a connection costs no extra list node. Protected by socks_mutex.
 *
 * New sockets go to the tail: a proxy backend is always created after its
 * client and so stays behind it, see deleteAllSockets(). */
static socket_info *sock_list_head;
static socket_info *sock_list_tail;
static rb_red_blk_tree *sock_tree;
static WS_MUTEX socks_mutex;

static void SockPrint(const void* a) {
	printf("%i", *(int*) a);
}
static void SockInfoPrint( UNUSED_PARA void* a) {
}
#if 0
static void SockInfoDest( UNUSED_PARA void *a) {
}
#endif
static void SockDest( UNUSED_PARA void* a) {
}

static int SockComp(const void* a, const void* b) {
	int ret;
	char *p_a = (char*) a;
	char *p_b = (char*) b;
	ret = strcmp(p_a, p_b);
	if (ret < 0){
		return -1;
	}
	if (ret > 0){
		return 1;
	}
	return 0;
}

/* Caller holds socks_mutex. Safe for a socket that is not in the list. */
static void sock_list_unlink(socket_info* sock) {
	if (sock->in_sock_list == 0) {
		return;
	}
	if (sock->sock_list_prev != NULL) {
		sock->sock_list_prev->sock_list_next = sock->sock_list_next;
	} else {
		sock_list_head = sock->sock_list_next;
	}
	if (sock->sock_list_next != NULL) {
		sock->sock_list_next->sock_list_prev = sock->sock_list_prev;
	} else {
		sock_list_tail = sock->sock_list_prev;
	}
	sock->sock_list_prev = NULL;
	sock->sock_list_next = NULL;
	sock->in_sock_list = 0;
}

void initSocketContainer(void) {
	PlatformCreateMutex(&socks_mutex);
	sock_tree = RBTreeCreate(SockComp, SockDest, SockDest, SockPrint, SockInfoPrint);
	sock_list_head = NULL;
	sock_list_tail = NULL;
}

void freeSocketContainer(void) {
	deleteAllSockets();
	if ( sock_tree != 0 ){
		RBTreeDestroy(sock_tree);
	}
}

void deleteSocket(socket_info* sock) {

	PlatformLockMutex(&socks_mutex);

	PlatformLockMutex(&sock->socket_mutex);
#ifdef ENABLE_DEVEL_WARNINGS
	#warning mal tracen wo das aufgerufen wird
#endif

	/* Under socket_mutex: sendWebsocketFrameReal() checks it under the same
	 * mutex, so a thread that still got hold of the socket sees it */
	sock->closeSocket = 1;
	sock_list_unlink(sock);
#ifdef WEBSERVER_USE_WEBSOCKETS
	if (sock->isWebsocket == 1) {
		rb_red_blk_node* node;
		node = RBExactQuery(sock_tree, sock->websocket_guid);
		if ( node != 0 ){
			RBDelete(sock_tree, node);
		}else{
			printf("Error: websocket giud %s nicht gefunden\n",sock->websocket_guid);
		}
	}
#endif

	PlatformUnlockMutex(&sock->socket_mutex);

	PlatformUnlockMutex(&socks_mutex);
}

void addSocketContainer(socket_info* sock) {
	PlatformLockMutex(&socks_mutex);
	if (sock->in_sock_list == 0) {
		sock->sock_list_prev = sock_list_tail;
		sock->sock_list_next = NULL;
		if (sock_list_tail != NULL) {
			sock_list_tail->sock_list_next = sock;
		} else {
			sock_list_head = sock;
		}
		sock_list_tail = sock;
		sock->in_sock_list = 1;
	}
	PlatformUnlockMutex(&socks_mutex);
}

#ifdef WEBSERVER_USE_WEBSOCKETS

void addSocketByGUID(socket_info* sock) {
	PlatformLockMutex(&socks_mutex);
	if (sock->isWebsocket == 1){
		RBTreeInsert(sock_tree, sock->websocket_guid, sock);
	}
	PlatformUnlockMutex(&socks_mutex);
}

socket_info* getSocketByGUID(const char* guid) {
	rb_red_blk_node* node;
	socket_info* sock = 0;

	PlatformLockMutex(&socks_mutex);

	node = RBExactQuery(sock_tree, (char*) guid);
	if (node != 0) {
		sock = (socket_info*) node->info;
		PlatformLockMutex(&sock->socket_mutex);
	}

	PlatformUnlockMutex(&socks_mutex);

	return sock;
}

#endif

void deleteAllSockets(void) {
	socket_info* sock;

	PlatformLockMutex(&socks_mutex);
	for (sock = sock_list_head; sock != NULL; sock = sock->sock_list_next) {
		PlatformShutdown(sock->socket);
	}
	PlatformUnlockMutex(&socks_mutex);

	/* Always close the current head instead of walking a snapshot: closing a
	 * proxy client also frees its backend socket, which a snapshot would
	 * still hold and close a second time. The backend sits behind its client
	 * (tail insertion), so it is normally gone before it would come up here;
	 * reverse_proxy_cleanup() copes with the other order as well. Unlinking
	 * first guarantees progress; the unlink in deleteSocket() is then a
	 * no-op. */
	for (;;) {
		PlatformLockMutex(&socks_mutex);
		sock = sock_list_head;
		if (sock != NULL) {
			sock_list_unlink(sock);
		}
		PlatformUnlockMutex(&socks_mutex);
		if (sock == NULL) {
			break;
		}
		WebserverConnectionManagerCloseRequest(sock);
	}
}

unsigned long dumpSocketsSize(int *count) {
	int i = 0;
	unsigned long size = 0;
	socket_info *sock;

	PlatformLockMutex(&socks_mutex);

	for (sock = sock_list_head; sock != NULL; sock = sock->sock_list_next) {
		size += getSocketInfoSize(sock);
		i++;
	}
	if (count != 0){
		*count = i;
	}

	PlatformUnlockMutex(&socks_mutex);
	return size;
}

void dumpSockets(http_request* s) {
	socket_info* sock;
	const char *type_str;
	const char *ssl_str;
	const char *close_str;
	const char *ws_str;

	PlatformLockMutex(&socks_mutex);

	for (sock = sock_list_head; sock != NULL; sock = sock->sock_list_next) {
		/* Determine socket type */
		if (sock->server == 1) {
			type_str = "Server";
		} else if (sock->client == 1) {
			type_str = sock->client_ip_str;
		} else {
			type_str = "-";
		}

		/* SSL status */
#ifdef WEBSERVER_USE_SSL
		ssl_str = (sock->use_ssl == 1) ? "yes" : "-";
#else
		ssl_str = "-";
#endif

		/* Close status */
		close_str = (sock->closeSocket == 1) ? "yes" : "-";

		/* WebSocket status */
#ifdef WEBSERVER_USE_WEBSOCKETS
		ws_str = (sock->isWebsocket == 1) ? "yes" : "-";
#else
		ws_str = "-";
#endif

		printHTMLChunk(s->socket,
			"<tr>"
				"<td>%s</td>"
				"<td>%d</td>"
				"<td>%"PRIu32"</td>"
				"<td>%s</td>"
				"<td>%s</td>"
				"<td>%s</td>"
			"</tr>",
			type_str, sock->port, getSocketInfoSize(sock),
			ssl_str, close_str, ws_str);
	}

	PlatformUnlockMutex(&socks_mutex);
}

