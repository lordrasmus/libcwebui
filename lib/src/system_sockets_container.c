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

#include <errno.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__NuttX__)
#include <sys/resource.h>
#include <time.h>
#endif
#if defined(__ZEPHYR__)
#include <zephyr/kernel.h>
#endif


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
/* Sockets in the list that are not listeners: what the total limit counts */
static uint32_t sock_list_clients;
/* Where connLimitPickGlobalVictim() continues; moved on when that socket
 * leaves the list */
static socket_info *global_scan_cursor;

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
	if (global_scan_cursor == sock) {
		global_scan_cursor = sock->sock_list_next;
	}
	if (sock->server == 0) {
		sock_list_clients--;
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
	sock_list_clients = 0;
	global_scan_cursor = NULL;
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
		if (sock->server == 0) {
			sock_list_clients++;
		}
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

/*
 * Limits and idle timeouts (seconds), 0 switches one off. Read on every
 * accept or once per second by the sweep, without the config store's name
 * lookup; written through setConfigInt() -> connLimitSetConfig(), possibly
 * from another thread, hence the atomic accesses. The defaults here hold
 * until initConfig() sets the same values.
 */
static int conn_limit_max = 64;					/* max_connections_per_ip */
static int conn_total_max = 0;					/* max_connections_total, 0 = automatic */
static int conn_fd_reserve = 128;				/* max_connections_fd_reserve */
static int to_request_header = 30;				/* connect / TLS handshake / request header */
static int to_keep_alive = 120;					/* between requests; above Firefox' 115 s */
static int to_receive_idle = 120;				/* request body / upload without a byte */
static int to_send_idle = 120;					/* response / file without a byte going out */
static int to_ws_ping_idle = 60;				/* websocket silent this long -> ping */
static int to_ws_pong = 30;						/* no reaction to the ping -> close */
static int to_proxy_connect = 10;				/* backend connect and request */
static int to_proxy_backend_header = 300;		/* backend response header (long polling) */
static int to_closing = 5;						/* marked for closing, nothing left to send */

/* Descriptor limit of the process, read once in connLimitInit(); 0 =
 * unknown or unlimited, then there is no automatic total limit. Capped, so
 * that it fits an int whatever the platform reports. */
static int conn_fd_limit = 0;
#define CONN_FD_LIMIT_CAP	(1 << 20)

#if defined(__GNUC__)
	#define CONN_CFG_GET(v)		__atomic_load_n(&(v), __ATOMIC_RELAXED)
	#define CONN_CFG_SET(v, x)	__atomic_store_n(&(v), (x), __ATOMIC_RELAXED)
#else
	#define CONN_CFG_GET(v)		(v)
	#define CONN_CFG_SET(v, x)	((v) = (x))
#endif

static const struct {
	const char *name;
	int *value;
} conn_config[] = {
	{ "max_connections_per_ip",			&conn_limit_max },
	{ "max_connections_total",			&conn_total_max },
	{ "max_connections_fd_reserve",		&conn_fd_reserve },
	{ "timeout_request_header",			&to_request_header },
	{ "timeout_keep_alive",				&to_keep_alive },
	{ "timeout_receive_idle",			&to_receive_idle },
	{ "timeout_send_idle",				&to_send_idle },
	{ "timeout_websocket_ping",			&to_ws_ping_idle },
	{ "timeout_websocket_pong",			&to_ws_pong },
	{ "timeout_proxy_connect",			&to_proxy_connect },
	{ "timeout_proxy_backend_header",	&to_proxy_backend_header },
	{ "timeout_closing",				&to_closing },
};

int connLimitSetConfig(const char* name, int value) {
	size_t i;
	for (i = 0; i < sizeof(conn_config) / sizeof(conn_config[0]); i++) {
		if (strcmp(name, conn_config[i].name) == 0) {
			CONN_CFG_SET(*conn_config[i].value, (value < 0) ? 0 : value);
			return 1;
		}
	}
	return 0;
}

void connLimitSetMax(int max_per_address) {
	CONN_CFG_SET(conn_limit_max, (max_per_address < 0) ? 0 : max_per_address);
}

int connLimitGetMax(void) {
	return CONN_CFG_GET(conn_limit_max);
}

uint32_t connLimitTotalCount(void) {
	return sock_list_clients;
}

/* Configured, or automatic: the descriptor limit minus the reserve for
 * everything that is not a connection (listeners, epoll, log, watchdog,
 * files, handlers: ~50 on the target, plus headroom). The reserve takes at
 * most half of a small limit, so that one still leaves connections. */
int connLimitTotalMax(void) {
	int configured = CONN_CFG_GET(conn_total_max);
	int reserve;

	if (configured > 0) {
		return configured;
	}
	if (conn_fd_limit == 0) {
		return 0;
	}
	reserve = CONN_CFG_GET(conn_fd_reserve);
	if (reserve > conn_fd_limit / 2) {
		reserve = conn_fd_limit / 2;
	}
	return conn_fd_limit - reserve;
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

	/* A valid loop time before the first connection is accepted */
	if (g_loop_now == 0) {
		g_loop_now = connMonotonicSec();
	}

	/* For the automatic total limit, see connLimitTotalMax(). Not raised:
	 * on the target the memory limit of the process would come before a
	 * higher descriptor limit. */
	conn_fd_limit = 0;
#if defined(RLIMIT_NOFILE)
	{
		struct rlimit rl;
		if ((getrlimit(RLIMIT_NOFILE, &rl) == 0) && (rl.rlim_cur != RLIM_INFINITY)) {
			conn_fd_limit = (rl.rlim_cur > (rlim_t)CONN_FD_LIMIT_CAP) ?
			                CONN_FD_LIMIT_CAP : (int)rl.rlim_cur;
		}
	}
#endif

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


/*
 * Idle timeouts and the total limit
 * =================================
 *
 * A connection must not stay forever in a state the server cannot end on
 * its own: a client not reading a response, an upload or a websocket
 * without progress, a request header trickling in byte by byte. And many
 * source addresses together must not take all descriptors.
 *
 * Time: once per event loop round, never per transfer. g_loop_now is set
 * right after every wait; a transfer only copies it into rx_at / tx_at. In
 * seconds - every deadline is 5..300 s - and +1, so that 0 stays free for
 * "not set". Time the loop spent blocked in a handler beyond one second
 * (flashing a firmware, a slow handler) does not count for anybody: it is
 * subtracted, otherwise every other connection would look idle afterwards.
 *
 * Check: once per second, after the events of that round have been
 * dispatched, a sweep over all sockets (~1000 at most, a few fields each).
 * Cheaper than any timer structure that had to be maintained on every
 * transfer, and with nothing to keep in sync on state changes: the state is
 * read from the socket, like connLimitIsEvictable() does.
 *
 * Closing happens after the walk and outside socks_mutex, at most
 * CONN_SWEEP_MAX per round (the rest in the next round right away). This is
 * safe because closing a client releases nothing but its own proxy backend,
 * and backends are never picked.
 */

uint32_t g_loop_now;

static uint32_t loop_stall;			/* blocked time taken out of g_loop_now */
static uint32_t loop_returned_at;	/* clock when the last wait returned */
static uint32_t loop_last_sweep;
static int loop_sweep_pending;		/* the last sweep had more to do */

#define CONN_SWEEP_MAX		64

uint32_t connMonotonicSec(void) {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__NuttX__)
	struct timespec ts;
#if defined(CLOCK_MONOTONIC_COARSE)
	/* Tick resolution is plenty, and it is the cheaper clock */
	if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) != 0)
#endif
	{
		if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
			return 0;
		}
	}
	return (uint32_t)ts.tv_sec + 1;
#elif defined(_WIN32)
	return (uint32_t)(GetTickCount64() / 1000) + 1;
#elif defined(__ZEPHYR__)
	return (uint32_t)(k_uptime_get() / 1000) + 1;
#else
	/* No clock known for this platform: timeouts stay off */
	return 0;
#endif
}

int eventLoopBeforeWait(void) {
	uint32_t mono = connMonotonicSec();

	if (mono == 0) {
		return -1;
	}
	if (loop_returned_at != 0) {
		uint32_t busy = mono - loop_returned_at;
		if (busy > 1) {
			loop_stall += busy - 1;
		}
	}
	if (loop_sweep_pending) {
		return 0;
	}
	/* Without connections nothing can time out: no extra wakeups */
	return (sock_list_clients > 0) ? 1000 : -1;
}

/* Runs between the wait and its error check in every backend, so it must
 * leave errno as the wait left it: a failed clock call with a working
 * fallback would otherwise turn an EINTR into a fatal error */
void eventLoopAfterWait(void) {
	int saved_errno = errno;
	uint32_t mono = connMonotonicSec();
	uint32_t now;

	loop_returned_at = mono;
	if (mono == 0) {
		g_loop_now = 0;
	} else {
		now = mono - loop_stall;
		g_loop_now = (now != 0) ? now : 1;
	}
	errno = saved_errno;
}

/* Whether the deadline `limit` seconds after `since` has passed; never for
 * a switched off limit or an unset stamp */
static int conn_expired(uint32_t now, uint32_t since, int limit) {
	return (limit > 0) && (since != 0) && ((uint32_t)(now - since) >= (uint32_t)limit);
}

static uint32_t conn_later(uint32_t a, uint32_t b) {
	return ((int32_t)(a - b) > 0) ? a : b;
}

enum {
	CONN_KEEP = 0,
	CONN_CLOSE,				/* close */
	CONN_CLOSE_IF_IDLE,		/* close unless input has arrived meanwhile */
	CONN_PING				/* send a websocket ping */
};

static int conn_timeout_proxy(socket_info* sock, reverse_proxy_connection_t* proxy, uint32_t now) {
	/* State changes are noticed here instead of at the many assignments in
	 * reverse_proxy.c; one second off is irrelevant for these deadlines */
	if (proxy->state != proxy->timed_state) {
		proxy->timed_state = proxy->state;
		proxy->state_at = now;
	}
	switch (proxy->state) {
	case PROXY_STATE_RECV_CLIENT_HEADER:
		/* A fixed deadline; input waiting buys at most one second, see
		 * connTimeoutSweep() */
		return conn_expired(now, sock->phase_at, CONN_CFG_GET(to_request_header)) ? CONN_CLOSE_IF_IDLE : CONN_KEEP;
	case PROXY_STATE_RECV_CLIENT_BODY:
		return conn_expired(now, sock->rx_at, CONN_CFG_GET(to_receive_idle)) ? CONN_CLOSE_IF_IDLE : CONN_KEEP;
	case PROXY_STATE_CONNECT_BACKEND:
	case PROXY_STATE_SEND_TO_BACKEND:
		/* The non-blocking connect is still running in SEND_TO_BACKEND */
		return conn_expired(now, conn_later(proxy->state_at, proxy->backend_at),
		                    CONN_CFG_GET(to_proxy_connect)) ? CONN_CLOSE : CONN_KEEP;
	case PROXY_STATE_RECV_BACKEND_HEADER:
		return conn_expired(now, proxy->state_at, CONN_CFG_GET(to_proxy_backend_header)) ? CONN_CLOSE : CONN_KEEP;
	case PROXY_STATE_STREAMING:
	case PROXY_STATE_STREAMING_WAIT_WRITE:
		/* Progress in either direction of the stream: backend delivering or
		 * client taking it */
		return conn_expired(now, conn_later(sock->tx_at, proxy->backend_at),
		                    CONN_CFG_GET(to_send_idle)) ? CONN_CLOSE : CONN_KEEP;
	default:
		/* Websocket tunnel: the two ends have their own ways to tell a dead
		 * peer; done/error: on their way out */
		return CONN_KEEP;
	}
}

#ifdef WEBSERVER_USE_WEBSOCKETS
/*
 * closing / out_waiting / ping_out: closeSocket, whether frames are queued
 * and whether the last ping has been written, read under socket_mutex - the
 * websocket threads and the write path change them under it.
 */
static int conn_timeout_websocket(socket_info* sock, uint32_t now, int closing, int out_waiting,
                                  int ping_out) {
	/* Frames waiting: the client has to take them, however busy it is
	 * sending. Queueing happens in the output thread, so the start is when
	 * the sweep first sees them - one second late at most. */
	if (out_waiting) {
		if (sock->ws_out_since == 0) {
			sock->ws_out_since = now;
		} else if (conn_expired(now, conn_later(sock->ws_out_since, sock->tx_at),
		                        CONN_CFG_GET(to_send_idle))) {
			return CONN_CLOSE;
		}
	} else {
		sock->ws_out_since = 0;
	}
	if (closing) {
		/* Queued frames still go out, new ones are refused; nothing to ping
		 * for. Once the queue is empty the closing deadline applies. */
		return out_waiting ? CONN_KEEP :
		       (conn_expired(now, conn_later(sock->rx_at, sock->tx_at),
		                     CONN_CFG_GET(to_closing)) ? CONN_CLOSE : CONN_KEEP);
	}
	if (sock->ws_ping_at != 0) {
		/* Anything received after the ping answers it, not only the pong */
		if ((int32_t)(sock->rx_at - sock->ws_ping_at) >= 0) {
			sock->ws_ping_at = 0;
			sock->ws_ping_sent_at = 0;
			return CONN_KEEP;
		}
		/* The answer is due from the ping having gone out, and only the
		 * client's own reaction counts then. Sending progress does not: a
		 * client whose application hangs still has its kernel take small
		 * periodic frames for a long time. While the ping waits behind
		 * other frames, the send_idle check above covers a client that
		 * takes nothing; once out, frames queued after it change nothing. */
		if (sock->ws_ping_sent_at == 0) {
			if (ping_out) {
				sock->ws_ping_sent_at = now;
			}
			return CONN_KEEP;
		}
		return conn_expired(now, sock->ws_ping_sent_at, CONN_CFG_GET(to_ws_pong)) ? CONN_CLOSE : CONN_KEEP;
	}
	return conn_expired(now, sock->rx_at, CONN_CFG_GET(to_ws_ping_idle)) ? CONN_PING : CONN_KEEP;
}
#endif

static int conn_in_tls_handshake(const socket_info* sock) {
#ifdef WEBSERVER_USE_SSL
	return sock->run_ssl_accept != 0;
#else
	(void)sock;
	return 0;
#endif
}

/*
 * What the sweep does with a socket. Decided from the socket's own fields;
 * the only writes are the bookkeeping stamps of the proxy and websocket
 * branches. The order matters (a websocket upgrade's 101 sits in
 * output_list with isWebsocket already set). Whatever is not recognised is
 * kept.
 */
static int conn_timeout_action(socket_info* sock, uint32_t now) {
	if (sock->server) {
		return CONN_KEEP;
	}
	if (sock->extern_handle == NULL && sock->header == NULL) {
		/* Not a client connection: a proxy backend before its handler is
		 * set, or something unknown */
		return CONN_KEEP;
	}
	if (!ws_list_empty(&sock->output_list)) {
		/* A response or file going out, whether closing afterwards or not */
		return conn_expired(now, sock->tx_at, CONN_CFG_GET(to_send_idle)) ? CONN_CLOSE : CONN_KEEP;
	}
#ifdef WEBSERVER_USE_WEBSOCKETS
	if (sock->isWebsocket) {
		/* Set once by the event loop, the rest is shared with the
		 * websocket threads. Lock order socks_mutex -> socket_mutex. */
		int closing;
		int out_waiting;
		int ping_out;
		PlatformLockMutex(&sock->socket_mutex);
		closing = sock->closeSocket;
		out_waiting = !ws_list_empty(&sock->websocket_chunk_list);
		/* Its chunk gone, or its chunk at the head and written past it */
		ping_out = ((int32_t)(sock->ws_chunks_sent - sock->ws_ping_seq) >= 0) ||
		           ((sock->ws_chunks_sent + 1 == sock->ws_ping_seq) &&
		            ((uint32_t)sock->websocket_send_pos >= sock->ws_ping_end));
		PlatformUnlockMutex(&sock->socket_mutex);
		return conn_timeout_websocket(sock, now, closing, out_waiting, ping_out);
	}
#endif
	if (sock->closeSocket) {
		/* Marked for closing, nothing left to send, but no event came to
		 * do it */
		return conn_expired(now, conn_later(sock->rx_at, sock->tx_at),
		                    CONN_CFG_GET(to_closing)) ? CONN_CLOSE : CONN_KEEP;
	}
	if (sock->extern_handle != NULL) {
		if ((sock->extern_handle == reverse_proxy_client_handler) &&
		    (sock->extern_handle_data_ptr != NULL)) {
			return conn_timeout_proxy(sock, (reverse_proxy_connection_t*) sock->extern_handle_data_ptr, now);
		}
		/* Proxy backend (lives and dies with its client) or a handler
		 * this code does not know */
		return CONN_KEEP;
	}
	if (sock->post_streaming_active || sock->header->header_complete) {
		/* Request body: streamed upload or buffered POST */
		return conn_expired(now, sock->rx_at, CONN_CFG_GET(to_receive_idle)) ? CONN_CLOSE_IF_IDLE : CONN_KEEP;
	}
	if ((sock->header_buffer_pos > 0) || (sock->header->parsed_bytes > 0) ||
	    conn_in_tls_handshake(sock) || (sock->bytes_received == 0)) {
		/* Connecting, TLS handshake, waiting for the first request, or a
		 * request header coming in: one fixed deadline for the whole phase,
		 * so that trickling bytes does not stretch it. Input waiting right
		 * at the deadline buys at most one second (a request arriving in
		 * that very second is not thrown away), see connTimeoutSweep(). */
		return conn_expired(now, sock->phase_at, CONN_CFG_GET(to_request_header)) ? CONN_CLOSE_IF_IDLE : CONN_KEEP;
	}
	/* Keep-alive between two requests */
	return conn_expired(now, sock->tx_at, CONN_CFG_GET(to_keep_alive)) ? CONN_CLOSE_IF_IDLE : CONN_KEEP;
}

#ifdef WEBSERVER_USE_WEBSOCKETS
/* A ping frame without payload, appended behind whatever is queued: frames
 * are appended whole under the same mutex, so it never lands inside one */
static void conn_send_ping(socket_info* sock, uint32_t now) {
	static const unsigned char ping[2] = { 0x89, 0x00 };
	int was_empty;

	PlatformLockMutex(&sock->socket_mutex);
	if (sock->closeSocket == 0) {
		was_empty = ws_list_empty(&sock->websocket_chunk_list);
		sendWebsocketChunk(sock, ping, sizeof(ping));
		if (was_empty) {
			activateEventSocketWrite(sock);
		}
		/* The ping ends in the last chunk, whether a new one or appended
		 * to the one there: it is out once all chunks before that one are,
		 * and that one up to the ping's end. Frames queued later may be
		 * appended to the same chunk, hence the position. */
		sock->ws_ping_seq = sock->ws_chunks_sent +
		                    (uint32_t)ws_list_size(&sock->websocket_chunk_list);
		sock->ws_ping_end = ((const html_chunk*)sock->websocket_chunk_list.last->data)->length;
		sock->ws_ping_at = now;
		sock->ws_ping_sent_at = 0;
	}
	PlatformUnlockMutex(&sock->socket_mutex);
}
#endif

static void connTimeoutSweep(uint32_t now) {
	socket_info *close_list[CONN_SWEEP_MAX];
	char close_if_idle[CONN_SWEEP_MAX];
	socket_info *ping_list[CONN_SWEEP_MAX];
	int n_close = 0;
	int n_ping = 0;
	int i;
	socket_info *sock;

	loop_sweep_pending = 0;

	PlatformLockMutex(&socks_mutex);
	for (sock = sock_list_head; sock != NULL; sock = sock->sock_list_next) {
		int action = conn_timeout_action(sock, now);
		if (action == CONN_KEEP) {
			if (sock->spared_at != 0) {
				/* The input it was spared for has been read */
				sock->spared_at = 0;
			}
			continue;
		}
		if ((action == CONN_CLOSE_IF_IDLE) && (sock->spared_at != 0) &&
		    ((uint32_t)(now - sock->spared_at) < 2)) {
			/* Spared already: the loop gets at least one whole second to
			 * read its input - two ticks, spared just before one would
			 * otherwise leave it milliseconds. Not collected meanwhile, an
			 * immediate further sweep would otherwise find it again. */
			continue;
		}
		if (action == CONN_PING) {
			if (n_ping < CONN_SWEEP_MAX) {
				ping_list[n_ping++] = sock;
			} else {
				loop_sweep_pending = 1;
			}
			continue;
		}
		if (n_close == CONN_SWEEP_MAX) {
			loop_sweep_pending = 1;
			break;
		}
		close_list[n_close] = sock;
		close_if_idle[n_close] = (char)(action == CONN_CLOSE_IF_IDLE);
		n_close++;
	}
	PlatformUnlockMutex(&socks_mutex);

	/* Pings first: nothing has been freed yet */
#ifdef WEBSERVER_USE_WEBSOCKETS
	for (i = 0; i < n_ping; i++) {
		conn_send_ping(ping_list[i], now);
	}
#else
	(void)ping_list;
#endif

	for (i = 0; i < n_close; i++) {
		sock = close_list[i];
		if (close_if_idle[i] && (sock->spared_at == 0) && connHasPendingInput(sock)) {
			/* Its request is here, it just has not been read yet. Once, for
			 * at least a whole second: under load one dispatch round need
			 * not reach it (at most MAX_EVENTS per round), a second of
			 * rounds normally does - a bound, not a proof. Still unread
			 * after that, it is closed: without a limit a socket nothing
			 * reads would stay for ever. */
			sock->spared_at = now;
			continue;
		}
		WebserverConnectionManagerCloseRequest(sock);
	}
}

void eventLoopAfterDispatch(void) {
	if (g_loop_now == 0) {
		return;
	}
	if ((g_loop_now != loop_last_sweep) || loop_sweep_pending) {
		loop_last_sweep = g_loop_now;
		connTimeoutSweep(g_loop_now);
	}
}

/*
 * A connection to close for the total limit: the evictable one with the
 * oldest transfer among the next CONN_GLOBAL_WINDOW client sockets after a
 * cursor that goes round the list. An approximation on purpose - no scan of
 * all connections per accept under a flood, and no structure to maintain on
 * every transfer; whatever is really idle for long is taken by the sweep
 * anyway. The cursor moves on even when nothing is found, so the next call
 * looks at other connections. NULL if there is none in two windows.
 */
#define CONN_GLOBAL_WINDOW	64

socket_info* connLimitPickGlobalVictim(void) {
	socket_info *best = NULL;
	socket_info *sock;
	int looked = 0;
	int steps = 0;

	PlatformLockMutex(&socks_mutex);
	sock = (global_scan_cursor != NULL) ? global_scan_cursor : sock_list_head;
	while ((sock != NULL) && (looked < 2 * CONN_GLOBAL_WINDOW) && (steps < (int)sock_list_clients + 8)) {
		socket_info *next = (sock->sock_list_next != NULL) ? sock->sock_list_next : sock_list_head;
		steps++;
		/* Listeners and proxy backends are passed without counting */
		if ((sock->server == 0) && !((sock->extern_handle == NULL) && (sock->header == NULL)) &&
		    (sock->extern_handle != reverse_proxy_backend_handler)) {
			looked++;
			if (connLimitIsEvictable(sock) && ((best == NULL) || (sock->last_io < best->last_io))) {
				best = sock;
			}
			if ((best != NULL) && (looked >= CONN_GLOBAL_WINDOW)) {
				sock = next;
				break;
			}
		}
		sock = next;
	}
	global_scan_cursor = sock;
	PlatformUnlockMutex(&socks_mutex);
	return best;
}
