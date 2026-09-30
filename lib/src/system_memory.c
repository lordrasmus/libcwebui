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


list_t chunk_cache;



Parameter* WebserverMallocParameter(void) {
	Parameter* ret = (Parameter*)WebserverMalloc(sizeof(Parameter));
	ret->name = 0;
	ret->value = 0;
	return ret;
}

void WebserverFreeParameter(Parameter* para) {
	if (para->name != 0) {
		WebserverFree(para->name);
	}
	if (para->value != 0) {
		WebserverFree(para->value);
	}
	WebserverFree(para);
}


static void freeChunkList(list_t* liste) {
	html_chunk* chunk;
	ws_list_iterator_start(liste);
	while ((chunk = (html_chunk*)ws_list_iterator_next(liste))) {
		WebserverFreeHtml_chunk(chunk);
	}
	ws_list_iterator_stop(liste);
	ws_list_destroy(liste);
}


socket_info* WebserverMallocSocketInfo(void) {
	socket_info* sock = (socket_info*)WebserverMalloc(sizeof(socket_info));
	memset(sock, 0, sizeof(socket_info));
	/* Create the mutex here rather than at each call site: every socket_info
	 * ends up in deleteSocket() (locks it) and WebserverFreeSocketInfo()
	 * (destroys it), so it always has to be initialised. Callers used to do
	 * this themselves and two of them forgot, leaving an all-zero mutex that
	 * only works by accident on pthread platforms. */
	PlatformCreateMutex(&sock->socket_mutex);
	ws_list_init(&sock->header_chunk_list);
	ws_list_init(&sock->html_chunk_list);
	ws_list_init(&sock->output_list);
#ifdef WEBSERVER_USE_WEBSOCKETS
	ws_list_init(&sock->websocket_chunk_list);
	ws_list_init(&sock->websocket_fragments);
#endif
	return sock;
}




void WebserverFreeSocketInfo(socket_info* sock) {
	/* Hand the pointer to the event backend while it is still valid: a batch
	 * of ready events may still hold entries pointing at this socket. This is
	 * the only place a socket_info is released, so it also covers the proxy
	 * backend socket, which is freed further down via reverse_proxy_cleanup()
	 * and reaches this function through the recursive call. */
	eventSocketFreed(sock);

	/* Bricht die Verbindung mitten im Upload ab, muss der Streaming Handler
	 * noch die Gelegenheit bekommen seine Teildatei aufzuraeumen. */
	postStreamFinish(sock, 0);
	cleanupSocketPostStreamState(sock);

	if (sock->header_buffer != 0) {
		WebserverFree(sock->header_buffer);
		sock->header_buffer = 0;
	}
	if (sock->header != 0) {
		WebserverFreeHttpRequestHeader(sock->header);
		sock->header = 0;
	}

#ifdef WEBSERVER_USE_WEBSOCKETS
	if (sock->isWebsocket == 1) {
		WebserverFree(sock->s);
		sock->s = 0;
	}

	freeChunkList(&sock->websocket_chunk_list);
	WebserverFree(sock->websocket_buffer);
	WebserverFree(sock->websocket_guid);
	WebserverFree(sock->websocket_store_guid);

	/* WebSocket-Fragmente und Streaming-Kontext beim Disconnect freigeben —
	 * sonst bleiben Referenzen auf geschlossene Verbindungen haengen und
	 * sammeln sich (Speicher + Event-Verarbeitung fuer tote Verbindungen). */
	{
		unsigned char* frag;
		while ((frag = (unsigned char*)ws_list_extract_at(&sock->websocket_fragments, 0)) != NULL) {
			WebserverFree(frag);
		}
	}
	if (sock->websocket_stream_ctx != NULL) {
		freeWebsocketStreamContext(sock->websocket_stream_ctx);
		sock->websocket_stream_ctx = NULL;
	}
#endif

	freeChunkList(&sock->header_chunk_list);
	freeChunkList(&sock->html_chunk_list);

#ifdef ENABLE_DEVEL_WARNINGS
	#warning "ssl strukturen freigeben"
#endif


	// TODO das löschen der output liste mal mit dem List freer lösen
	while ( ws_list_size( &sock->output_list ) > 0 ){
		output_struct *output = (output_struct*)ws_list_get_at( &sock->output_list, 0 );

		ws_list_delete( &sock->output_list, output );

		if( output != 0 ){
			if ( output->header.buffer != 0 ){
				WebserverFree( output->header.buffer );
			}

			if ( output->body.buffer != 0 ){
				WebserverFree( output->body.buffer );
			}
		}
		WebserverFree( output );

	}

	reverse_proxy_cleanup(sock);

	PlatformDestroyMutex(&sock->socket_mutex);

	WebserverFree(sock);


}





Cookie* WebserverMallocCookie(void) {
	Cookie* ret = (Cookie*)WebserverMalloc(sizeof(Cookie));
	memset(ret, 0, sizeof(Cookie));
	return ret;
}

void WebserverFreeCookie(Cookie* cookie) {
#ifdef _WEBSERVER_COOKIE_DEBUG_
	if (cookie == 0) {
		WebServerPrintf("Free Cookie ( %p )\n", cookie);
	}
	else {
		WebServerPrintf("Free Cookie ( %p ) Name <%s>  Value <%s> \n", cookie, cookie->name, cookie->value);
	}
#endif
	if (cookie == 0) {
		return;
	}
	if (cookie->name != 0) {
		WebserverFree(cookie->name);
	}
	if (cookie->value != 0) {
		WebserverFree(cookie->value);
	}
	WebserverFree(cookie);
}


ws_variable* WebserverMallocVariable_store(void) {
	ws_variable* ret = (ws_variable*)WebserverMalloc(sizeof(ws_variable));
	memset(ret, 0, sizeof(ws_variable));
	return ret;
}

void WebserverFreeVariable_store(ws_variable* store) {
	WebserverFree(store);
}

sessionStore* WebserverMallocSessionStore(void) {
	sessionStore* ret = (sessionStore*)WebserverMalloc(sizeof(sessionStore));
	memset(ret, 0, sizeof(sessionStore));
	ret->vars = createVariableStore();
	return ret;
}

void WebserverFreeSessionStore(sessionStore* store) {
	deleteVariableStore(store->vars);
	WebserverFree(store);
}

static void *WebserverFreeCookieFreer(const void *free_element) {
	WebserverFreeCookie((Cookie*)free_element);
	return 0;
}




HttpRequestHeader* WebserverMallocHttpRequestHeader(void) {
	HttpRequestHeader* header = (HttpRequestHeader*)WebserverMalloc(sizeof(HttpRequestHeader));
	memset(header, 0, sizeof(HttpRequestHeader));
	header->parameter_store = createVariableStore();
	ws_list_init(&header->cookie_list);
	ws_list_attributes_freer(&header->cookie_list, WebserverFreeCookieFreer);
	return header;
}

/***********************************************************************************************
*                                                                                              *
*          Gesamtbudget fuer gepufferte POST Bodies                                            *
*                                                                                              *
************************************************************************************************/

/* max_post_size begrenzt den einzelnen Request. Das reicht nicht: der Puffer
 * wird sofort in voller Content-Length alloziert, sobald das erste Byte
 * eintrifft. Ein Client kann also mit N parallelen Verbindungen N * max_post_size
 * binden, ohne die Daten je zu senden. Darum ein zweiter Deckel ueber die Summe
 * aller Verbindungen.
 *
 * Streaming Handler ( RegisterPostStreamHandler ) puffern nichts und tauchen
 * hier nie auf - der Firmware Upload ist damit ohne Sonderfall ausgenommen. */
static uint64_t post_buffer_allocated = 0;
static WS_MUTEX post_buffer_mutex;

/* Eager beim Start anlegen, nicht lazy beim ersten Gebrauch: ein
 * check-then-act ohne Synchronisierung waere genau dann kaputt, wenn hier
 * einmal zwei Threads ankommen. Alle anderen Subsysteme ( mem_mutex,
 * initSessions, initSocketContainer ) machen es ebenso. */
void initPostBufferBudget(void) {
	PlatformCreateMutex(&post_buffer_mutex);
}

void freePostBufferBudget(void) {
	PlatformDestroyMutex(&post_buffer_mutex);
	post_buffer_allocated = 0;
}

/* 1 = Budget reicht und ist belegt, 0 = erschoepft */
int postBufferReserve(uint64_t size) {
	uint64_t limit;
	int ok = 0;

	limit = (uint64_t)getConfigInt("max_post_memory_total");
	if (limit == 0) {
		return 1;   /* Budget abgeschaltet */
	}

	PlatformLockMutex(&post_buffer_mutex);
	if ((post_buffer_allocated + size) <= limit) {
		post_buffer_allocated += size;
		ok = 1;
	}
	PlatformUnlockMutex(&post_buffer_mutex);

	return ok;
}

void postBufferRelease(uint64_t size) {
	if (size == 0) return;

	PlatformLockMutex(&post_buffer_mutex);
	if (post_buffer_allocated >= size) {
		post_buffer_allocated -= size;
	} else {
		post_buffer_allocated = 0;   /* darf nicht unterlaufen */
	}
	PlatformUnlockMutex(&post_buffer_mutex);
}

uint64_t postBufferAllocated(void) {
	uint64_t ret;
	PlatformLockMutex(&post_buffer_mutex);
	ret = post_buffer_allocated;
	PlatformUnlockMutex(&post_buffer_mutex);
	return ret;
}

void WebserverResetHttpRequestHeader(HttpRequestHeader *header) {

	clearVariables(header->parameter_store);
	ws_list_destroy(&header->cookie_list);

	header->parsed_bytes = 0;

	header->post_buffer_pos = 0;
	header->contenttype = 0;
	header->contentlenght = 0;
	header->method = 0;

	if (header->url != 0) {
		WebserverFree(header->url);
		header->url = 0;
	}
	if (header->If_Modified_Since != 0) {
		WebserverFree(header->If_Modified_Since);
		header->If_Modified_Since = 0;
	}
	if (header->etag != 0) {
		WebserverFree(header->etag);
		header->etag = 0;
	}
	if (header->Host != 0) {
		WebserverFree(header->Host);
		header->Host = 0;
	}
	if (header->HostName != 0) {
		WebserverFree(header->HostName);
		header->HostName = 0;
	}
	if (header->Connection != 0) {
		WebserverFree(header->Connection);
		header->Connection = 0;
	}
	if (header->Upgrade != 0) {
		WebserverFree(header->Upgrade);
		header->Upgrade = 0;
	}
	if (header->Origin != 0) {
		WebserverFree(header->Origin);
		header->Origin = 0;
	}

	if (header->post_buffer != 0) {
		WebserverFree(header->post_buffer);
		header->post_buffer = 0;
	}

	/* Bewusst nicht an post_buffer gekoppelt: wird der Puffer irgendwo anders
	 * freigegeben und genullt, bliebe die Reservierung sonst bis zum Neustart
	 * im Gesamtbudget stehen und der Server nimmt irgendwann keine POSTs mehr an. */
	if (header->post_buffer_reserved != 0) {
		postBufferRelease(header->post_buffer_reserved);
		header->post_buffer_reserved = 0;
	}

	if (header->boundary != 0) {
		WebserverFree(header->boundary);
		header->boundary = 0;
	}

	if (header->Content_Disposition != 0) {
		WebserverFree(header->Content_Disposition);
		header->Content_Disposition = 0;
	}

	if (header->Content_Type != 0) {
		WebserverFree(header->Content_Type);
		header->Content_Type = 0;
	}

	if (header->Access_Control_Request_Method != 0) {
		WebserverFree(header->Access_Control_Request_Method);
		header->Access_Control_Request_Method = 0;
	}

	if (header->Access_Control_Request_Headers != 0) {
		WebserverFree(header->Access_Control_Request_Headers);
		header->Access_Control_Request_Headers = 0;
	}

	if (header->Accept_Encoding != 0) {
		WebserverFree(header->Accept_Encoding);
		header->Accept_Encoding = 0;
	}

	if (header->UserAgent != 0) {
		WebserverFree(header->UserAgent);
		header->UserAgent = 0;
	}

	if (header->Referer != 0) {
		WebserverFree(header->Referer);
		header->Referer = 0;
	}

	if (header->Accept != 0) {
		WebserverFree(header->Accept);
		header->Accept = 0;
	}

	if (header->Authorization != 0) {
		WebserverFree(header->Authorization);
		header->Authorization = 0;
	}

#ifdef WEBSERVER_USE_WEBSOCKETS
	if (header->SecWebSocketKey != 0) {
		WebserverFree(header->SecWebSocketKey);
		header->SecWebSocketKey = 0;
	}
	
	if (header->SecWebSocketProtocol != 0) {
		WebserverFree(header->SecWebSocketProtocol);
		header->SecWebSocketProtocol = 0;
	}
#endif
}

void WebserverFreeHttpRequestHeader(HttpRequestHeader* header) {
	WebserverResetHttpRequestHeader(header);
	deleteVariableStore(header->parameter_store);
	ws_list_destroy(&header->cookie_list);
	WebserverFree(header);

}

FUNCTION_PARAS* WebserverMallocFunctionParas(void) {
	FUNCTION_PARAS *ret = 0;
	ret = (FUNCTION_PARAS*)WebserverMalloc(sizeof(FUNCTION_PARAS));
	memset(ret, 0, sizeof(FUNCTION_PARAS));
	return ret;
}

void WebserverFreeFunctionParas(FUNCTION_PARAS *func) {
	WebserverFree(func);
}

html_chunk* WebserverMallocHtml_chunk(void) {
	html_chunk* ret;
	if (ws_list_size(&chunk_cache) == 0) {
		ret = (html_chunk*)WebserverMalloc(sizeof(html_chunk));
		ret->text = (char*)WebserverMalloc(2000);
	}
	else {
		ret = (html_chunk*)ws_list_extract_at(&chunk_cache, 0);
	}
	ret->length = 0;
	return ret;
}

void WebserverFreeHtml_chunk(html_chunk* chunk) {
	/*if (chunk_cache.numels < 20) {
	ws_list_append(&chunk_cache, chunk);
	} else {*/
	if (chunk->text != 0) {
		WebserverFree(chunk->text);
	}
	WebserverFree(chunk);
	/*}*/
}
