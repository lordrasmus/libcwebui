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

/* Descriptors handed in with WebserverInjectExternFD(). They are not
 * connections: the descriptor stays the caller's, the server only watches
 * it. Kept here so that one can be removed again and its socket_info is
 * released on shutdown. Only a handful exist, a plain list does.
 *
 * Injection happens before WebserverStart(), when socks_mutex does not exist
 * yet, so this list has no lock: it is only touched before the event loop
 * starts and from the event loop thread. */
typedef struct extern_fd_entry {
	socket_info *sock;
	struct extern_fd_entry *next;
} extern_fd_entry;

static extern_fd_entry *extern_fds;
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
	connLimitInit();
}

void addExternSocket(socket_info* sock) {
	extern_fd_entry *entry = (extern_fd_entry*) WebserverMalloc(sizeof(extern_fd_entry));
	entry->sock = sock;
	entry->next = extern_fds;
	extern_fds = entry;
}

static void releaseExternSocket(extern_fd_entry *entry) {
	delEventSocketAll(entry->sock);
	/* Unregistered, not closed: the descriptor belongs to the caller */
	WebserverFreeSocketInfo(entry->sock);
	WebserverFree(entry);
}

int removeExternSocket(int fd) {
	extern_fd_entry **pos;
	for (pos = &extern_fds; *pos != NULL; pos = &(*pos)->next) {
		if ((*pos)->sock->socket == fd) {
			extern_fd_entry *entry = *pos;
			*pos = entry->next;
			releaseExternSocket(entry);
			return 0;
		}
	}
	return -1;
}

int countExternSockets(void) {
	int count = 0;
	extern_fd_entry *entry;
	for (entry = extern_fds; entry != NULL; entry = entry->next) {
		count++;
	}
	return count;
}

static void freeExternSockets(void) {
	while (extern_fds != NULL) {
		extern_fd_entry *entry = extern_fds;
		extern_fds = entry->next;
		releaseExternSocket(entry);
	}
}

void freeSocketContainer(void) {
	deleteAllSockets();
	freeExternSockets();
	connLimitFree();
	/* Under the lock and cleared, so a late GUID lookup finds no tree
	 * instead of a freed one */
	PlatformLockMutex(&socks_mutex);
	if ( sock_tree != 0 ){
		RBTreeDestroy(sock_tree);
		sock_tree = 0;
	}
	PlatformUnlockMutex(&socks_mutex);
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
	if ((sock->isWebsocket == 1) && (sock_tree != 0)) {
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
	if ((sock->isWebsocket == 1) && (sock_tree != 0)){
		RBTreeInsert(sock_tree, sock->websocket_guid, sock);
	}
	PlatformUnlockMutex(&socks_mutex);
}

socket_info* getSocketByGUID(const char* guid) {
	rb_red_blk_node* node;
	socket_info* sock = 0;

	PlatformLockMutex(&socks_mutex);

	node = (sock_tree != 0) ? RBExactQuery(sock_tree, (char*) guid) : 0;
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



/*
 * Per-address connection limit
 * ============================
 *
 * A single client must not be able to hold all connections the server can
 * take (descriptors, memory) and lock everybody else out. Connections are
 * grouped by source address; when a group is full, the next connection from
 * that address closes the one of its connections that has been idle the
 * longest. If none of them is idle, the new connection is refused.
 *
 * What counts as one source is decided by connLimitKeyFromAddress(): IPv4
 * and IPv4-mapped IPv6 by the address, global IPv6 by its /64 (a host can
 * pick any address in its /64), other IPv6 (link-local, ULA, loopback) by
 * the full address, so that all hosts on a segment do not share one group.
 *
 * Cost: in normal operation one hash lookup per accepted connection and a
 * counter increment per transfer. Finding the idle connection walks the
 * members of one group, and only when that group is at its limit. There is
 * deliberately no per-state bookkeeping that every state change would have
 * to maintain: connLimitIsEvictable() reads the state from the socket when
 * it is needed, and treats anything it does not recognise as busy.
 *
 * Not covered: a client that holds its slots in states that count as busy
 * - not reading a response, an upload or a websocket without progress -
 * keeps them; further connections from its address are refused.
 * It cannot take more than its share, though. Ending such connections needs
 * idle timeouts, which the server does not have yet.
 *
 * All of this runs in the event loop thread (accept, transfers, closing), so
 * the table has no lock. Only the limit itself can be set from any thread.
 */

#define CONN_GROUP_BUCKETS	256		/* power of two */

typedef struct conn_group_s {
	ws_peer_key key;
	uint32_t count;					/* connections in the group */
	uint32_t pinned;				/* held by an accept in progress */
	socket_info *members;
	struct conn_group_s *hash_next;	/* bucket chain, or free list */
} conn_group;

uint64_t g_conn_io_seq;

static conn_group *conn_buckets[CONN_GROUP_BUCKETS];
/* Released groups are kept for reuse instead of going back to the
 * allocator; there are never more than there are descriptors. */
static conn_group *conn_group_free;
static uint32_t conn_hash_key[2];
static int conn_hash_key_set;

/* Default 64; 0 switches the limit off. Read on every accept without the
 * config store's name lookup, written by setConfigInt(). */
static int conn_limit_max = 64;

void connLimitSetMax(int max_per_address) {
	if (max_per_address < 0) {
		max_per_address = 0;
	}
#if defined(__GNUC__)
	__atomic_store_n(&conn_limit_max, max_per_address, __ATOMIC_RELAXED);
#else
	conn_limit_max = max_per_address;
#endif
}

int connLimitGetMax(void) {
#if defined(__GNUC__)
	return __atomic_load_n(&conn_limit_max, __ATOMIC_RELAXED);
#else
	return conn_limit_max;
#endif
}

void connLimitKeyFromAddress(const void* addr, uint32_t addr_len, ws_peer_key* key) {
	const uint8_t *a = (const uint8_t*) addr;

	memset(key, 0, sizeof(*key));
	key->valid = 1;

	if (addr_len == 4) {
		/* ::ffff:a.b.c.d, the same form an IPv4-mapped IPv6 address has */
		key->addr[10] = 0xff;
		key->addr[11] = 0xff;
		memcpy(&key->addr[12], a, 4);
		return;
	}
	if (addr_len == 16) {
		static const uint8_t v4_mapped[12] = { 0,0,0,0, 0,0,0,0, 0,0,0xff,0xff };
		if (memcmp(a, v4_mapped, sizeof(v4_mapped)) == 0) {
			memcpy(key->addr, a, 16);
		} else if ((a[0] & 0xe0) == 0x20) {
			/* Global unicast 2000::/3: the /64 */
			memcpy(key->addr, a, 8);
		} else {
			memcpy(key->addr, a, 16);
		}
		return;
	}
	/* Unknown address type: one shared group (all zero is ::, never a peer
	 * address). Better limited together than not limited at all. */
}

/* HalfSipHash-1-3 over the 16 byte key, with a random key per process: an
 * attacker who picks source addresses cannot steer them into one bucket.
 * Made for 32 bit CPUs (32 bit words, rotations and additions only). */
#define CONN_ROTL(x, b)	(uint32_t)(((x) << (b)) | ((x) >> (32 - (b))))
#define CONN_HROUND \
	do { \
		v0 += v1; v1 = CONN_ROTL(v1, 5);  v1 ^= v0; v0 = CONN_ROTL(v0, 16); \
		v2 += v3; v3 = CONN_ROTL(v3, 8);  v3 ^= v2; \
		v0 += v3; v3 = CONN_ROTL(v3, 7);  v3 ^= v0; \
		v2 += v1; v1 = CONN_ROTL(v1, 13); v1 ^= v2; v2 = CONN_ROTL(v2, 16); \
	} while (0)

static uint32_t conn_hash(const ws_peer_key* key) {
	uint32_t v0 = 0, v1 = 0, v2 = 0x6c796765, v3 = 0x74656462;
	const uint32_t k0 = conn_hash_key[0], k1 = conn_hash_key[1];
	const uint32_t b = ((uint32_t)sizeof(key->addr)) << 24;
	size_t i;

	v3 ^= k1; v2 ^= k0; v1 ^= k1; v0 ^= k0;
	for (i = 0; i < sizeof(key->addr); i += 4) {
		uint32_t m = (uint32_t)key->addr[i] | ((uint32_t)key->addr[i + 1] << 8) |
		             ((uint32_t)key->addr[i + 2] << 16) | ((uint32_t)key->addr[i + 3] << 24);
		v3 ^= m;
		CONN_HROUND;
		v0 ^= m;
	}
	v3 ^= b;
	CONN_HROUND;
	v0 ^= b;
	v2 ^= 0xff;
	CONN_HROUND;
	CONN_HROUND;
	CONN_HROUND;
	return v1 ^ v3;
}

/* Called from initSocketContainer(), before the first accept: reading the
 * random source opens a descriptor, which may fail once they run short */
void connLimitInit(void) {
	char random[2 * WEBSERVER_GUID_LENGTH + 1];
	size_t i;

	/* Once per process: a new key would put groups already in the table
	 * into other buckets than the ones they hang in */
	if (conn_hash_key_set) {
		return;
	}

	PlatformGetGUID(random, sizeof(random));
	conn_hash_key[0] = 0x9e3779b9;
	conn_hash_key[1] = 0x85ebca6b;
	for (i = 0; random[i] != '\0'; i++) {
		uint32_t *k = &conn_hash_key[i & 1];
		*k = CONN_ROTL(*k, 5) ^ (uint8_t)random[i];
		*k *= 0x01000193;
	}
	conn_hash_key_set = 1;
}

struct conn_group_s* connLimitAcquire(const ws_peer_key* key) {
	conn_group *group;
	uint32_t bucket;

	if (conn_hash_key_set == 0) {
		/* Only without initSocketContainer(), e.g. in unit tests */
		connLimitInit();
	}
	bucket = conn_hash(key) & (CONN_GROUP_BUCKETS - 1);
	for (group = conn_buckets[bucket]; group != NULL; group = group->hash_next) {
		if (memcmp(group->key.addr, key->addr, sizeof(key->addr)) == 0) {
			group->pinned++;
			return group;
		}
	}

	if (conn_group_free != NULL) {
		group = conn_group_free;
		conn_group_free = group->hash_next;
	} else {
		group = (conn_group*) WebserverMalloc(sizeof(conn_group));
	}
	memset(group, 0, sizeof(*group));
	group->key = *key;
	group->pinned = 1;
	group->hash_next = conn_buckets[bucket];
	conn_buckets[bucket] = group;
	return group;
}

/* Back to the free list once nothing uses the group any more */
static void conn_group_put(conn_group* group) {
	conn_group **pos;

	if ((group->count != 0) || (group->pinned != 0)) {
		return;
	}
	pos = &conn_buckets[conn_hash(&group->key) & (CONN_GROUP_BUCKETS - 1)];
	while ((*pos != NULL) && (*pos != group)) {
		pos = &(*pos)->hash_next;
	}
	if (*pos != group) {
		/* Not where its key says - leave it rather than linking a bucket
		 * chain into the free list */
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "%s", "connection group not found in its bucket");
		return;
	}
	*pos = group->hash_next;
	group->hash_next = conn_group_free;
	conn_group_free = group;
}

void connLimitRelease(struct conn_group_s* group) {
	group->pinned--;
	conn_group_put(group);
}

uint32_t connLimitCount(const struct conn_group_s* group) {
	return group->count;
}

void connLimitAttach(struct conn_group_s* group, socket_info* sock) {
	sock->conn_group = group;
	sock->grp_prev = NULL;
	sock->grp_next = group->members;
	if (group->members != NULL) {
		group->members->grp_prev = sock;
	}
	group->members = sock;
	group->count++;
	CONN_LIMIT_NOTE_IO(sock);
	connLimitRelease(group);
}

void connLimitDetach(socket_info* sock) {
	conn_group *group = sock->conn_group;

	if (group == NULL) {
		return;
	}
	if (sock->grp_prev != NULL) {
		sock->grp_prev->grp_next = sock->grp_next;
	} else {
		group->members = sock->grp_next;
	}
	if (sock->grp_next != NULL) {
		sock->grp_next->grp_prev = sock->grp_prev;
	}
	sock->grp_prev = NULL;
	sock->grp_next = NULL;
	sock->conn_group = NULL;
	group->count--;
	conn_group_put(group);
}

/*
 * Whether a connection may be closed to make room for a new one from the same
 * address: 1 while the server is only waiting for the client - just accepted,
 * TLS handshake, keep-alive between requests, a request header coming in, a
 * request whose body has been announced but has not started (also through
 * the proxy) - and 0 while the server is working for it: a response or file
 * going out, an upload whose body has started (buffered or streamed), a
 * websocket, a proxied request past its header.
 *
 * Waiting states are evictable even while header bytes still trickle in:
 * otherwise a client sending part of a header, or a POST header and then
 * nothing, would make all of its slots untouchable. Which of the evictable
 * ones goes is decided by the time of their last transfer, so a request that
 * is actually arriving is not the one picked.
 *
 * The rule that keeps this safe: whatever is not positively recognised as
 * waiting is busy. A state this function does not know leads to a refused
 * new connection, never to a working one being cut off.
 */
int connLimitIsEvictable(const socket_info* sock) {
	if (sock->closeSocket) {
		return 0;
	}
#ifdef WEBSERVER_USE_WEBSOCKETS
	if (sock->isWebsocket) {
		return 0;
	}
#endif
	if (sock->post_streaming_active) {
		return 0;
	}
	if (!ws_list_empty(&sock->output_list)) {
		return 0;
	}
	if (sock->extern_handle != NULL) {
		/* Only the proxy is known, and only while it still receives the
		 * request from the client */
		if ((sock->extern_handle == reverse_proxy_client_handler) &&
		    (sock->extern_handle_data_ptr != NULL)) {
			const reverse_proxy_connection_t *proxy =
				(const reverse_proxy_connection_t*) sock->extern_handle_data_ptr;
			if (proxy->state == PROXY_STATE_RECV_CLIENT_HEADER) {
				return 1;
			}
			/* Body announced but not a byte of it yet; once it has started
			 * it is an upload in progress */
			if ((proxy->state == PROXY_STATE_RECV_CLIENT_BODY) &&
			    (proxy->request_body_received == 0)) {
				return 1;
			}
		}
		return 0;
	}
	if (sock->header == NULL) {
		return 0;
	}
	/* header_complete is only still set while the body of a buffered POST
	 * comes in. Header without any body yet: waiting. Body started: an upload
	 * in progress, not to be cut off. A body that starts and then stalls is
	 * left to the idle timeouts, like a stalled streamed upload. */
	if (sock->header->header_complete && (sock->header->post_buffer_pos > 0)) {
		return 0;
	}
	/* No response pending and no handler involved: the server waits for the
	 * client */
	return 1;
}

/* The evictable connection with the oldest last transfer, NULL if none */
socket_info* connLimitPickVictim(struct conn_group_s* group) {
	socket_info *sock;
	socket_info *best = NULL;

	for (sock = group->members; sock != NULL; sock = sock->grp_next) {
		if (!connLimitIsEvictable(sock)) {
			continue;
		}
		if ((best == NULL) || (sock->last_io < best->last_io)) {
			best = sock;
		}
	}
	return best;
}

void connLimitFree(void) {
	uint32_t i;

	/* Groups still in the table have members; only possible if sockets
	 * outlive the container, then they keep their group */
	for (i = 0; i < CONN_GROUP_BUCKETS; i++) {
		conn_group **pos = &conn_buckets[i];
		while (*pos != NULL) {
			conn_group *group = *pos;
			if ((group->count == 0) && (group->pinned == 0)) {
				*pos = group->hash_next;
				WebserverFree(group);
			} else {
				pos = &group->hash_next;
			}
		}
	}
	while (conn_group_free != NULL) {
		conn_group *group = conn_group_free;
		conn_group_free = group->hash_next;
		WebserverFree(group);
	}
}
