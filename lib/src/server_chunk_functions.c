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
#include "miniz_tdef.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

#ifdef ENABLE_DEVEL_WARNINGS
	#warning feste Buffer entfernen
#endif

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
		if (length < diff){
			to_write = length;
		}else{
			to_write = diff;
		}

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

}

void writeChunkVariable(list_t* liste, ws_variable* var) {
	char buffer[20];
	ws_variable* it;
	if (var == 0){
		return;
	}

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
	if (length == 0){
		return;
	}
	if (sock->disable_output == 1){
		return;
	}
	writeChunk(&sock->html_chunk_list, (unsigned char*) text, length);
}

void VISIBLE_ATTR sendHTMLChunkVariable(socket_info* sock, ws_variable* var) {
	if (sock->disable_output == 1){
		return;
	}
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

int isChunkListbigger(list_t* liste, int bytes){
	int bigger = 0;
	int count = 0;
	html_chunk* chunk;

	ws_list_iterator_start(liste);
	while ( ( chunk = (html_chunk*)ws_list_iterator_next(liste) )) {
		count += chunk->length;
		
		if ( count > bytes ){
			bigger = 1;
			break;
		}
	}
	ws_list_iterator_stop(liste);
	
	return bigger;
}


void vprintHeaderChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char tmp[1000];
	if (sock == 0){
		return;
	}
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
	if (sock == 0){
		return 0;
	}
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
	if (sock == 0){
		return;
	}
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


static char* get_status( tdefl_status status ){
	switch( status ){
		case TDEFL_STATUS_BAD_PARAM: return "TDEFL_STATUS_BAD_PARAM";
		case TDEFL_STATUS_OKAY: return "TDEFL_STATUS_OKAY";
		case TDEFL_STATUS_DONE: return "TDEFL_STATUS_DONE";
		case TDEFL_STATUS_PUT_BUF_FAILED: return "TDEFL_STATUS_PUT_BUF_FAILED";
	}
	
	return "unknown";
	
}
unsigned long writeChunksToBuffer(list_t* liste, char* out_buffer, int compress) {
	html_chunk* chunk;
	unsigned long offset = 0;
	tdefl_status status;
	tdefl_compressor *deflator;
	
	if ( compress == 1 ){
		int level = 5;
		// The number of dictionary probes to use at each compression level (0-10). 0=implies fastest/minimal possible probing.
		static const mz_uint s_tdefl_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };
		
		// create tdefl() compatible flags (we have to compose the low-level flags ourselves, or use tdefl_create_comp_flags_from_zip_params() but that means MINIZ_NO_ZLIB_APIS can't be defined).
		mz_uint comp_flags = s_tdefl_num_probes[MZ_MIN(10, level)] | ((level <= 3) ? TDEFL_GREEDY_PARSING_FLAG : 0);
		// mit level == 0 kann man noch das anhängen
		// 		comp_flags |= TDEFL_FORCE_ALL_RAW_BLOCKS;


		deflator = tdefl_compressor_alloc();
		// Initialize the low-level compressor.
		status = tdefl_init( deflator, NULL, NULL, comp_flags);
		if (status != TDEFL_STATUS_OKAY){
			printf("tdefl_init() failed!\n");
			return EXIT_FAILURE;
		}
	}

	ws_list_iterator_start(liste);
	while ( ( chunk = (html_chunk*) ws_list_iterator_next(liste) ) ) {
		if ( compress == 1 ){
			
			size_t in_bytes = chunk->length;
			size_t out_bytes = 1000000;
			// Compress as much of the input as possible (or all of it) to the output buffer.
			status = tdefl_compress( deflator, chunk->text, &in_bytes, &out_buffer[offset], &out_bytes, TDEFL_FULL_FLUSH);
			offset += out_bytes;
			if ( status != TDEFL_STATUS_OKAY ){
				printf("status : %s %ld\n",get_status(status),offset);
			}
		}else{
			memcpy(&out_buffer[offset], chunk->text, chunk->length);
			offset += chunk->length;
		}
		WebserverFreeHtml_chunk(chunk);
	}
	ws_list_iterator_stop(liste);
	
	
	if ( compress == 1 ){
		size_t in_bytes = 0;
		size_t out_bytes = 1000000;
		status = tdefl_compress( deflator, 0, &in_bytes, &out_buffer[offset], &out_bytes, TDEFL_FINISH);
		offset += out_bytes;
		if ( status != TDEFL_STATUS_DONE ){
			printf("status : %s %ld\n",get_status(status),offset);
		}
		tdefl_compressor_free( deflator );
	}
	
	ws_list_clear(liste);
	return offset;
}

void generateOutputBuffer(socket_info* sock) {
	char* buffer;
	unsigned long offset = 0;
	
	sock->file_infos.file_send_pos = 0;
	
	if ( sock->use_output_compression == 1 ){
		unsigned long body_size = getChunkListSize(&sock->html_chunk_list);
		buffer = (char*) WebserverMalloc ( body_size + 1  ); /* +1 fuer Header Debug '\0' */
		
		offset = writeChunksToBuffer(&sock->html_chunk_list, &buffer[offset], 1 );
		sock->output_main_buffer = buffer;
		sock->output_main_buffer_size = offset;
		
		if ( body_size == 0){
			printf("url : %s\n",sock->header->url);
		}
#if 0
		double proz = offset;
		proz /= (double)body_size;
		proz *= 100.0;
		printf("orig: %lu  compress: %lu  %.2f %%\n",body_size,offset,proz );
#endif
		
		printHeaderChunk(sock, "Content-Encoding: deflate\r\n");
		printHeaderChunk(sock, "Content-Length: %d\r\n", offset);
		printHeaderChunk(sock, "\r\n"); /* HTTP Header beenden */
		
		unsigned long header_size = getChunkListSize(&sock->header_chunk_list);
		buffer = (char*) WebserverMalloc ( header_size + 1  ); /* +1 fuer Header Debug '\0' */
		
		offset = writeChunksToBuffer(&sock->header_chunk_list, buffer, 0);
		
		sock->output_header_buffer = buffer;
		sock->output_header_buffer_size = offset;
		return;
	}
	
	unsigned long header_size = getChunkListSize(&sock->header_chunk_list);
	unsigned long body_size = getChunkListSize(&sock->html_chunk_list);
	
	unsigned long size = header_size + body_size;
	buffer = (char*) WebserverMalloc ( size + 1  ); /* +1 fuer Header Debug '\0' */

	offset = writeChunksToBuffer(&sock->header_chunk_list, buffer, 0);

#ifdef _WEBSERVER_HEADER_DEBUG_
	buffer[offset]='\0';
	LOG (HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket, "-------- sending Header --------\r\n %s -------- Header end --------",buffer );
#endif

	offset += writeChunksToBuffer(&sock->html_chunk_list, &buffer[offset], 0 );

#ifdef _WEBSERVER_BODY_DEBUG_
	WebServerPrintf ( "%s",buffer );
#endif

#ifdef _WEBSERVER_DEBUG_
	LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket, "compiled HTML Size %d ",size );
#endif
	sock->output_main_buffer = buffer;
	sock->output_main_buffer_size = offset;
	

}

