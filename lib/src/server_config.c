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


static ws_variable_store *config_v_store;

void initConfig(void) {
	config_v_store = createVariableStore();
	setConfigInt( "port",80);
	setConfigInt( "ssl_port",-1);
	setConfigInt( "max_post_size",2 * 1024 * 1024); // Default 2 MB Max Post Size
	/* Obergrenze fuer die Summe aller gleichzeitig gepufferten POST Bodies
	   ueber alle Verbindungen. max_post_size begrenzt nur den einzelnen
	   Request - ohne Gesamtbudget koennte ein Client mit vielen parallelen
	   Verbindungen beliebig viel Speicher binden, ohne auch nur die Daten zu
	   senden ( der Puffer wird sofort in voller Content-Length alloziert ).
	   Streaming Handler puffern nichts und zaehlen darum nicht mit. */
	setConfigInt( "max_post_memory_total", 16 * 1024 * 1024);
	setConfigInt( "use_csp",0);
	setConfigInt( "reload_py_modules",0);
	/* Connections one source address may hold at the same time; 0 = no
	   limit. The next one closes the longest idle connection of that
	   address, see system_sockets_container.c */
	setConfigInt( "max_connections_per_ip", 64);
	/* All connections together, including proxy backends; 0 = automatic,
	   the descriptor limit minus a reserve for everything else */
	setConfigInt( "max_connections_total", 0);
	/* Descriptors the automatic total limit leaves for everything else
	   (listeners, files, log, handlers); at most half of the limit */
	setConfigInt( "max_connections_fd_reserve", 128);
	/* Idle timeouts in seconds, 0 = off, see system_sockets_container.c.
	   request_header: from the connection or the start of a request until
	   its header is complete (TLS handshake included). keep_alive: between
	   two requests. receive_idle / send_idle: no byte of a request body /
	   response for that long. websocket_ping: a silent websocket gets a
	   ping, websocket_pong later it is closed. proxy_connect: backend
	   connect and request; proxy_backend_header: waiting for the backend's
	   answer (long polling). closing: marked for closing, nothing to send. */
	setConfigInt( "timeout_request_header", 30);
	setConfigInt( "timeout_keep_alive", 120);
	setConfigInt( "timeout_receive_idle", 120);
	setConfigInt( "timeout_send_idle", 120);
	setConfigInt( "timeout_websocket_ping", 60);
	setConfigInt( "timeout_websocket_pong", 30);
	setConfigInt( "timeout_proxy_connect", 10);
	setConfigInt( "timeout_proxy_backend_header", 300);
	setConfigInt( "timeout_closing", 5);
	#ifdef WEBSERVER_SESSION_TIMEOUT
		setConfigInt( "session_timeout" , WEBSERVER_SESSION_TIMEOUT );
	#else
		setConfigInt( "session_timeout" , 300 );
	#endif

	setConfigInt( "ssl_disable_SSLv2",1);
	setConfigInt( "ssl_disable_SSLv3",1);
	setConfigInt( "ssl_disable_TLSv1.0",0);
	setConfigInt( "ssl_disable_TLSv1.1",0);

	//setConfigText( "server_ip","127.0.0.1");
}

void freeConfig(void){
	deleteVariableStore(config_v_store);
}

void setConfigText(const char* name, const char* text) {
	ws_variable *var;
	PlatformLockMutex(&config_v_store->lock);
	var = newVariable(config_v_store, name, 0 );
	setWSVariableString(var, text);
	PlatformUnlockMutex(&config_v_store->lock);
}

void setConfigInt(const char* name, const int value) {
	ws_variable *var;
	if (value < -1) {
		LOG(CONFIG_LOG, WARNING_LEVEL, 0, "Config '%s' has suspicious negative value: %d", name, value);
	}
	PlatformLockMutex(&config_v_store->lock);
	var = newVariable(config_v_store, name, 0 );
	setWSVariableInt(var, value);
	/* Connection limits and timeouts are read on every accept or in the
	   once-per-second sweep - kept in plain variables so that no lookup by
	   name is needed there. Under the store's lock, so that two concurrent
	   setters cannot leave store and copy disagreeing. */
	connLimitSetConfig(name, value);
	PlatformUnlockMutex(&config_v_store->lock);
}

char* getConfigText(const char* name) {
	ws_variable* var;
	var = getVariable(config_v_store, name);
	if (var != 0){
		return var->val.value_string;
	}
	return 0;
}

int getConfigInt(const char* name) {
	ws_variable* var = getVariable(config_v_store, name);
	if (var != 0){
		return getWSVariableInt(var);
	}
	return 0;
}
