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

	/* Order of the last transfer on any connection; event loop thread only */
	extern uint64_t g_conn_io_seq;
	#define CONN_LIMIT_NOTE_IO(sock)	( (sock)->last_io = ++g_conn_io_seq )

#ifdef __cplusplus
}
#endif


#endif

