/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifndef _SYSTEM_SOCKETS_CONTAINER_H_
#define _SYSTEM_SOCKETS_CONTAINER_H_


#ifdef __cplusplus
extern "C" {
#endif

	void initSocketContainer(void);
	void freeSocketContainer(void);
	void deleteSocket(socket_info* sock);
	void deleteAllSockets(void);
	void addSocketContainer(socket_info* sock);
	void addSocketByGUID(socket_info* sock);
	socket_info* getSocketByGUID(const char* guid);

	/* Sockets of WebserverInjectExternFD(), see system_sockets_container.c */
	void addExternSocket(socket_info* sock);
	int  removeExternSocket(int fd);
	int  countExternSockets(void);

	/* Per-address connection limit, see system_sockets_container.c */
	struct conn_group_s;
	void connLimitInit(void);
	void connLimitFree(void);
	void connLimitSetMax(int max_per_address);
	int  connLimitGetMax(void);
	struct conn_group_s* connLimitAcquire(const ws_peer_key* key);
	void connLimitRelease(struct conn_group_s* group);
	uint32_t connLimitCount(const struct conn_group_s* group);
	socket_info* connLimitPickVictim(struct conn_group_s* group);
	void connLimitAttach(struct conn_group_s* group, socket_info* sock);
	void connLimitDetach(socket_info* sock);
	int  connLimitIsEvictable(const socket_info* sock);
	/* addr_len 4 (IPv4) or 16 (IPv6), anything else gives the shared key
	 * for unknown address types */
	void connLimitKeyFromAddress(const void* addr, uint32_t addr_len, ws_peer_key* key);

	/* Config values of the connection limits and idle timeouts, set through
	 * setConfigInt(); returns 1 if name is one of them */
	int  connLimitSetConfig(const char* name, int value);

	/* Total limit: all sockets in the container except listeners (proxy
	 * backends included, they are descriptors as well) */
	uint32_t connLimitTotalCount(void);
	int  connLimitTotalMax(void);
	socket_info* connLimitPickGlobalVictim(void);

	/* Idle timeouts, driven by the event backends around every wait:
	 * before (returns the wait timeout in ms, -1 = none), right after it,
	 * and after the events of that wait have been dispatched */
	int  eventLoopBeforeWait(void);
	void eventLoopAfterWait(void);
	void eventLoopAfterDispatch(void);
	uint32_t connMonotonicSec(void);

	/* Order of the last transfer on any connection; event loop thread only */
	extern uint64_t g_conn_io_seq;
	/* Event loop time in seconds, 0 = no clock (timeouts off) */
	extern uint32_t g_loop_now;
	#define CONN_LIMIT_NOTE_IO(sock)	( (sock)->last_io = ++g_conn_io_seq )
	#define CONN_NOTE_RX(sock)			( (sock)->last_io = ++g_conn_io_seq, (sock)->rx_at = g_loop_now )
	#define CONN_NOTE_TX(sock)			( (sock)->last_io = ++g_conn_io_seq, (sock)->tx_at = g_loop_now )

	/* system_sockets.c: input already arrived for sock but not read yet */
	int  connHasPendingInput(socket_info* sock);
	/* system_sockets.c: make room below the total limit by closing the
	 * longest idle connection; 1 if there is room afterwards */
	int  connLimitEnsureTotalRoom(void);

#ifdef __cplusplus
}
#endif


#endif

