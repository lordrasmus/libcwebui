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


list_t chunk_cache;

#if 0
static void WebserverFreeMem(void) {
	/*    for (i=0;i<g_files.FileCount;i++)
	{
	WebserverFree(g_files.files[i]->Name);
	WebserverFree(g_files.files[i]->Data);
	WebserverFree(g_files.files[i]);
	}
	WebserverFree(g_files.files);*/
	/* TODO: "freigeben der datai infos wieder einbauen" */
}
#endif



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
	ws_list_init(&sock->header_chunk_list);
	ws_list_init(&sock->html_chunk_list);
#ifdef WEBSERVER_USE_WEBSOCKETS
	ws_list_init(&sock->websocket_chunk_list);
	ws_list_init(&sock->websocket_fragments);
#endif
	ws_list_init(&sock->firephplogs);
	return sock;
}




void WebserverFreeSocketInfo(socket_info* sock) {
	if (sock->header_buffer != 0) {
		WebserverFree(sock->header_buffer);
		sock->header_buffer = 0;
	}
	if (sock->header != 0) {
		WebserverFreeHttpRequestHeader(sock->header);
		sock->header = 0;
	}

#ifdef USE_LIBEVENT
	if (sock->my_ev != 0) {
		deleteEvent(sock);
	}
#endif
#ifdef WEBSERVER_USE_WEBSOCKETS
	if (sock->isWebsocket == 1) {
		WebserverFree(sock->s);
		sock->s = 0;
	}

	freeChunkList(&sock->websocket_chunk_list);
	WebserverFree(sock->websocket_buffer);
	WebserverFree(sock->websocket_guid);
	WebserverFree(sock->websocket_store_guid);
#endif

	freeChunkList(&sock->header_chunk_list);
	freeChunkList(&sock->html_chunk_list);

#ifdef ENABLE_DEVEL_WARNINGS
	#warning "ssl strukturen freigeben"
#endif

		ws_list_destroy(&sock->firephplogs);

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

void WebserverResetHttpRequestHeader(HttpRequestHeader *header) {

	clearVariables(header->parameter_store);
	ws_list_destroy(&header->cookie_list);


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


#ifdef WEBSERVER_USE_WEBSOCKETS
	if (header->SecWebSocketKey1 != 0) {
		WebserverFree(header->SecWebSocketKey1);
		header->SecWebSocketKey1 = 0;
	}
	if (header->SecWebSocketKey2 != 0) {
		WebserverFree(header->SecWebSocketKey2);
		header->SecWebSocketKey2 = 0;
	}
	if (header->SecWebSocketKey != 0) {
		WebserverFree(header->SecWebSocketKey);
		header->SecWebSocketKey = 0;
	}
	if (header->SecWebSocketOrigin != 0) {
		WebserverFree(header->SecWebSocketOrigin);
		header->SecWebSocketOrigin = 0;
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
