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

#include "stdafx.h"
#include "webserver.h"

#include "red_black_tree.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

static list_t sock_list;
static rb_red_blk_tree *sock_tree;
static WS_MUTEX socks_mutex;

void SockPrint(const void* a) {
	printf("%i", *(int*) a);
}
void SockInfoPrint( UNUSED_PARA void* a) {
}
void SockInfoDest( UNUSED_PARA void *a) {
}
void SockDest( UNUSED_PARA void* a) {
}

int SockComp(const void* a, const void* b) {
	int ret;
	char *p_a = (char*) a;
	char *p_b = (char*) b;
	ret = strcmp(p_a, p_b);
	if (ret < 0) return -1;
	if (ret > 0) return 1;
	return 0;
}

void initSocketContainer(void) {
	PlatformCreateMutex(&socks_mutex);
	sock_tree = RBTreeCreate(SockComp, SockDest, SockDest, SockPrint, SockInfoPrint);
	ws_list_init(&sock_list);
}

void freeSocketContainer(void) {
	deleteAllSockets();
	ws_list_destroy(&sock_list);
	RBTreeDestroy(sock_tree);
}

void deleteSocket(socket_info* sock) {
	rb_red_blk_node* node;
	PlatformLockMutex(&socks_mutex);

	PlatformLockMutex(&sock->socket_mutex);
#ifdef ENABLE_DEVEL_WARNINGS
	#warning mal tracen wo das aufgerufen wird
#endif

	ws_list_delete(&sock_list, sock);
#ifdef WEBSERVER_USE_WEBSOCKETS
	if (sock->isWebsocket == 1) {
		node = RBExactQuery(sock_tree, sock->websocket_guid);
		RBDelete(sock_tree, node);
	}
#endif

	PlatformUnlockMutex(&sock->socket_mutex);

	PlatformUnlockMutex(&socks_mutex);
}

void addSocketContainer(socket_info* sock) {
	PlatformLockMutex(&socks_mutex);
	ws_list_append(&sock_list, sock);
	PlatformUnlockMutex(&socks_mutex);
}

#ifdef WEBSERVER_USE_WEBSOCKETS

void addSocketByGUID(socket_info* sock) {
	PlatformLockMutex(&socks_mutex);
	if (sock->isWebsocket == 1) RBTreeInsert(sock_tree, sock->websocket_guid, sock);
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
	list_t del_list;

	ws_list_init(&del_list);

	PlatformLockMutex(&socks_mutex);

	ws_list_iterator_start(&sock_list);
	while (ws_list_iterator_hasnext(&sock_list)) {
		sock = (socket_info*) ws_list_iterator_next(&sock_list);
		shutdown(sock->socket,SHUT_RDWR);
		ws_list_append(&del_list, sock);
	}
	ws_list_iterator_stop(&sock_list);

	PlatformUnlockMutex(&socks_mutex);

	ws_list_iterator_start(&del_list);
	while (ws_list_iterator_hasnext(&del_list)) {
		sock = (socket_info*) ws_list_iterator_next(&del_list);
		WebserverConnectionManagerCloseRequest(sock);
	}
	ws_list_iterator_stop(&del_list);

	ws_list_destroy(&del_list);

}

unsigned long dumpSocketsSize(int *count) {
	int i = 0;
	unsigned long size = 0;
	socket_info *sock;

	PlatformLockMutex(&socks_mutex);

	ws_list_iterator_start(&sock_list);
	while ((sock = (socket_info*) ws_list_iterator_next(&sock_list))) {
		size += getSocketInfoSize(sock);
		i++;
	}
	ws_list_iterator_stop(&sock_list);
	if (count != 0) *count = i;

	PlatformUnlockMutex(&socks_mutex);
	return size;
}

void dumpSockets(http_request* s) {
	socket_info* sock;

	PlatformLockMutex(&socks_mutex);

	ws_list_iterator_start(&sock_list);



	while ((sock = (socket_info*) ws_list_iterator_next(&sock_list))) {
		printHTMLChunk(s->socket, "<tr><td>");

		if (sock->server == 1) {
			printHTMLChunk(s->socket, "Server Socket");
		}
		if (sock->client == 1) {
			printHTMLChunk(s->socket, "Client Socket %s", sock->client_ip_str);
		}

		printHTMLChunk(s->socket, "<td>%d<td>%d<td>", sock->port, getSocketInfoSize(sock));

		if (sock->use_ssl == 1) {
			printHTMLChunk(s->socket, "active ");
		}


		printHTMLChunk(s->socket, "<td>");
		if (sock->closeSocket == 1) {
			printHTMLChunk(s->socket, "close ");
		}

#ifdef WEBSERVER_USE_WEBSOCKETS
		printHTMLChunk(s->socket, "<td>");
		if (sock->isWebsocket == 1) {
			printHTMLChunk(s->socket, "active ");
		}
#endif

	}



	ws_list_iterator_stop(&sock_list);
	PlatformUnlockMutex(&socks_mutex);

}

