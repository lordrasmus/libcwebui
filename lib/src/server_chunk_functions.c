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

#ifdef __GNUC__
	#include "webserver.h"
#endif

#warning feste Buffer entfernen

void WebserverPrintShortInfos(void);

void freeHTMLChunk(void* var) {
	/*   html_chunk* chunk = ( html_chunk* ) var;
	 WebserverFree ( chunk->text );
	 WebserverFree ( chunk );
	 */
	WebserverFreeHtml_chunk((html_chunk*)var);
}

void writeChunk(list_t* liste, const unsigned char* text, unsigned int length) {
	html_chunk* chunk = 0, *tmp;
	unsigned int to_write, diff, offset = 0;
	if (ws_list_size(liste) == 0) {
		chunk = WebserverMallocHtml_chunk();
		ws_list_append(liste, chunk);
	}

	if (chunk == 0) {
		tmp = (html_chunk*)ws_list_get_at(liste, ws_list_size(liste) - 1);
		if ( ( tmp != 0 ) && (tmp->length <= 2000) ) {
			chunk = tmp;
		}
	}

	while (length > 0) {
		if (chunk == 0) {
			chunk = WebserverMallocHtml_chunk();
			ws_list_append(liste, chunk);
		}
		diff = 2000 - chunk->length;
		if (length < diff)
			to_write = length;
		else
			to_write = diff;

		memcpy(&chunk->text[chunk->length], &text[offset], to_write);
		chunk->length += to_write;
		length -= to_write;
		offset += to_write;
		chunk = 0;
	}

	/*if ((*liste) != 0) {
	 tmp = (html_chunk*) (*liste)->value;
	 if ((tmp->length + length) <= 2000) {
	 chunk = tmp;
	 if (chunk->text == 0) {
	 printf("Null Chunk Text ???\n");
	 }
	 }
	 }

	 */

	//return chunk;
}

void writeChunkVariable(list_t* liste, ws_variable* var) {
	char buffer[20];
	ws_variable* it;
	if (var == 0) return;

	switch (var->type) {
	case VAR_TYPE_STRING:
		writeChunk(liste, (unsigned char*) var->val.value_string, var->extra.str_len);
		break;
	case VAR_TYPE_INT:
		getWSVariableString(var, buffer, 20);
		writeChunk(liste, (unsigned char*) buffer, strlen(buffer));
		break;
	case VAR_TYPE_ULONG:
		getWSVariableString(var, buffer, 20);
		writeChunk(liste, (unsigned char*) buffer, strlen(buffer));
		break;
	case VAR_TYPE_REF:
		writeChunkVariable(liste, var->val.value_ref);
		break;
	case VAR_TYPE_ARRAY:
		writeChunk(liste, (unsigned char*) "Array <br> ", 10);
		it = getWSVariableArrayFirst( var );
		while( it != 0 ){
			writeChunk(liste, (unsigned char*) "[", 1);
			writeChunk(liste, (unsigned char*) it->name, it->name_len);
			writeChunk(liste, (unsigned char*) ":", 1);
			writeChunkVariable( liste , it );
			writeChunk(liste, (unsigned char*) "]<br>", 5);
			it = getWSVariableArrayNext( var );
		}
		break;
	case VAR_TYPE_EMPTY:
		break;

	case VAR_TYPE_CUSTOM_DATA:
		writeChunk(liste, (unsigned char*)"custom_data", strlen("custom_data") );
		break;
	}
}

void sendHeaderChunk(socket_info* sock, const char* text, const unsigned int length) {
	writeChunk(&sock->header_chunk_list, (unsigned char*) text, length);
}

void sendHTMLChunk(socket_info* sock, const char* text, const unsigned int length) {
	//html_chunk* chunk=0;//,*tmp;
	if (length == 0) return;
	if (sock->disable_output == 1) return;
	writeChunk(&sock->html_chunk_list, (unsigned char*) text, length);
}

void sendHTMLChunkVariable(socket_info* sock, ws_variable* var) {
	if (sock->disable_output == 1) return;
	writeChunkVariable(&sock->html_chunk_list, var);
}

#ifdef WEBSERVER_USE_WEBSOCKETS
void sendWebsocketChunk(socket_info* sock, const unsigned char* text, const unsigned int length) {
	writeChunk(&sock->websocket_chunk_list, text, length);
}
#endif

unsigned long getChunkListSize(list_t* liste) {
	unsigned long ret = 0;
	html_chunk* chunk;

	ws_list_iterator_start(liste);
	while ( ( chunk = (html_chunk*)ws_list_iterator_next(liste) )) {
		ret += chunk->length;
	}
	ws_list_iterator_stop(liste);
	return ret;
}


void vprintHeaderChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char tmp[1000];
	if (sock == 0) return;
	l = vsnprintf(tmp, 1000, fmt, argptr);
	writeChunk(&sock->header_chunk_list, (unsigned char*) tmp, l);
}

void printHeaderChunk(socket_info* sock, const char *fmt, ... ) {
	va_list argptr;
	va_start ( argptr, fmt );
	vprintHeaderChunk(sock,fmt,argptr);
	va_end ( argptr );
}

int vprintHTMLChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char tmp[1000];
	if (sock == 0) return 0;
	l = vsnprintf(tmp, 1000, fmt, argptr);
	writeChunk(&sock->html_chunk_list, (unsigned char*) tmp, l);
	return l;
}

int printHTMLChunk(socket_info* sock, const char *fmt, ... ) {
	int ret=0;
	va_list argptr;
	va_start(argptr,fmt);
	ret = vprintHTMLChunk(sock,fmt,argptr);
	va_end(argptr);
	return ret;
}

#ifdef WEBSERVER_USE_WEBSOCKETS

void vprintWebsocketChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char tmp[1000];
	if (sock == 0) return;
	l = vsnprintf(tmp, 1000, fmt, argptr);
	writeChunk(&sock->websocket_chunk_list, (unsigned char*) tmp, l);
}

void printWebsocketChunk(socket_info* sock, const char *fmt, ... ) {
	va_list argptr;
	va_start(argptr,fmt);
	vprintWebsocketChunk(sock,fmt,argptr);
	va_end(argptr);
}

#endif

unsigned long writeChunksToBuffer(list_t* liste, char* out_buffer) {
	html_chunk* chunk;
	unsigned long offset = 0;

	ws_list_iterator_start(liste);
	while ( ( chunk = (html_chunk*) ws_list_iterator_next(liste) ) ) {
		memcpy(&out_buffer[offset], chunk->text, chunk->length);
		offset += chunk->length;
		WebserverFreeHtml_chunk(chunk);
	}
	ws_list_iterator_stop(liste);

	ws_list_clear(liste);
	return offset;
}

void generateOutputBuffer(socket_info* sock) {
	//ListNode* liste;
	//html_chunk* chunk;
	char* buffer;
	unsigned long offset = 0;
	unsigned long size = getChunkListSize(&sock->header_chunk_list) + getChunkListSize(&sock->html_chunk_list);
	buffer = (char*) WebserverMalloc ( size + 1  ); // +1 fuer Header Debug '\0'

	offset = writeChunksToBuffer(&sock->header_chunk_list, buffer);

#ifdef _WEBSERVER_HEADER_DEBUG_
	buffer[offset]='\0';
	LOG (HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket, "-------- sending Header --------\r\n %s -------- Header end --------",buffer );
#endif

	offset += writeChunksToBuffer(&sock->html_chunk_list, &buffer[offset]);

#ifdef _WEBSERVER_BODY_DEBUG_
	WebServerPrintf ( "%s",buffer );
#endif

#ifdef _WEBSERVER_DEBUG_
	LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket, "compiled HTML Size %d ",size );
#endif
	sock->output_buffer = buffer;
	sock->output_buffer_size = size;
	sock->file_infos.file_send_pos = 0;

}

