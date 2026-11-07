/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifdef __CDT_PARSER__
	#define __BASE_FILE__ base
#endif

#include "webserver.h"
#include <miniz.h>

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

#ifdef ENABLE_DEVEL_WARNINGS
	#warning feste Buffer entfernen
#endif



static void writeChunk(list_t* liste, const unsigned char* text, unsigned int length) {
	html_chunk* chunk = 0, *tmp;
	unsigned int to_write, diff, offset = 0;
	if (ws_list_size(liste) == 0) {
		chunk = WebserverMallocHtml_chunk();
		ws_list_append(liste, chunk,0);
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
			ws_list_append(liste, chunk,0);
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

static void writeChunkVariable(list_t* liste, ws_variable* var) {
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
		getWSVariableString(var, buffer, sizeof( buffer) );
		writeChunk(liste, (unsigned char*) buffer, strlen(buffer));
		break;
	case VAR_TYPE_ULONG:
		getWSVariableString(var, buffer, sizeof( buffer));
		writeChunk(liste, (unsigned char*) buffer, strlen(buffer));
		break;
    case VAR_TYPE_LONG:
		getWSVariableString(var, buffer, sizeof( buffer));
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

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
static void vprintHeaderChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char *tmp;
	va_list argcopy;
	
	va_copy(argcopy, argptr);

	l = vsnprintf( 0, 0, fmt, argptr);
	tmp = WebserverMalloc( l + 1 ); // +1 wegen dem \0
	
	l = vsnprintf(tmp, l +1, fmt, argcopy);
	
	
	writeChunk(&sock->header_chunk_list, (unsigned char*) tmp, l);

	WebserverFree( tmp );
	va_end(argcopy);
}
#pragma GCC diagnostic warning "-Wformat-nonliteral"

void printHeaderChunk(socket_info* sock, const char *fmt, ... ) {
	va_list argptr;
	
	if (sock == 0){
		return;
	}
	
	va_start ( argptr, fmt );
	vprintHeaderChunk(sock,fmt,argptr);
	va_end ( argptr );
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
int vprintHTMLChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char *tmp;
	va_list argcopy;
	
	if (sock == 0){
		return 0;
	}
	
	// Kopie von argptr erstellen. ansonsten funktionert das zweite vsnprintf() nicht
	// weil argptr schon auf dem letzten argument steht
	va_copy(argcopy, argptr);

	l = vsnprintf( 0, 0, fmt, argptr);
	tmp = WebserverMalloc( l + 1 ); // +1 wegen dem \0

	l = vsnprintf(tmp, l + 1 , fmt, argcopy);

	writeChunk(&sock->html_chunk_list, (unsigned char*) tmp, l );
	WebserverFree( tmp );
	va_end(argcopy);
	return l;
}
#pragma GCC diagnostic warning "-Wformat-nonliteral"

int printHTMLChunk(socket_info* sock, const char *fmt, ... ) {
	int ret=0;
	va_list argptr;
	va_start(argptr,fmt);
	ret = vprintHTMLChunk(sock,fmt,argptr);
	va_end(argptr);
	return ret;
}

#ifdef WEBSERVER_USE_WEBSOCKETS

static void vprintWebsocketChunk(socket_info* sock, const char *fmt, va_list argptr) {
	int l;
	char *tmp;
	va_list argcopy;
	
	if (sock == 0){
		return;
	}
	
	va_copy(argcopy, argptr);
	
	l = vsnprintf( 0, 0, fmt, argptr);
	tmp = WebserverMalloc( l + 1 ); // +1 wegen dem \0
	
	l = vsnprintf(tmp, l + 1, fmt, argcopy);
	
	writeChunk(&sock->websocket_chunk_list, (unsigned char*) tmp, l);

	WebserverFree( tmp );
	va_end(argcopy);
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
static void releaseChunks(list_t* liste) {
	html_chunk* chunk;

	ws_list_iterator_start(liste);
	while ( ( chunk = (html_chunk*) ws_list_iterator_next(liste) ) ) {
		WebserverFreeHtml_chunk(chunk);
	}
	ws_list_iterator_stop(liste);
	ws_list_clear(liste);
}

/* Copies the chunks into out_buffer, which must hold getChunkListSize(),
 * and releases them */
static unsigned long writeChunksToBuffer(list_t* liste, char* out_buffer) {
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

/*
 * Room deflateChunks() may need for body_size bytes in chunk_count chunks.
 * The base is miniz' own bound (mz_deflateBound(), not available with
 * MINIZ_NO_ZLIB_APIS): incompressible data goes out as stored blocks and
 * grows a little. On top, every chunk is flushed on its own, which ends a
 * block and adds an empty stored block as sync marker: up to 5 bytes of
 * block header, 5 of marker and the padding to the byte boundary.
 */
static unsigned long deflateCapacity(unsigned long body_size, unsigned int chunk_count) {
	unsigned long a = 128 + (body_size / 100) * 110 + ((body_size % 100) * 110) / 100;
	unsigned long b = 128 + body_size + ((body_size / (31 * 1024)) + 1) * 5;
	return MZ_MAX(a, b) + (unsigned long)chunk_count * 16;
}

/*
 * Compresses the chunks into out_buffer without releasing them. Returns the
 * compressed size, 0 when that did not work - the chunks are still there
 * for sending the page uncompressed.
 */
static unsigned long deflateChunks(list_t* liste, char* out_buffer, unsigned long capacity) {
	html_chunk* chunk;
	unsigned long offset = 0;
	tdefl_status status;
	tdefl_compressor *deflator;
	int failed = 0;
	int level = 5;
	// The number of dictionary probes to use at each compression level (0-10). 0=implies fastest/minimal possible probing.
	static const mz_uint s_tdefl_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };

	// create tdefl() compatible flags (we have to compose the low-level flags ourselves, or use tdefl_create_comp_flags_from_zip_params() but that means MINIZ_NO_ZLIB_APIS can't be defined).
	mz_uint comp_flags = s_tdefl_num_probes[MZ_MIN(10, level)] | ((level <= 3) ? TDEFL_GREEDY_PARSING_FLAG : 0);

	deflator = tdefl_compressor_alloc();
	if (deflator == NULL) {
		return 0;
	}
	status = tdefl_init( deflator, NULL, NULL, comp_flags);
	if (status != TDEFL_STATUS_OKAY){
		printf("tdefl_init() failed: %s\n", get_status(status));
		tdefl_compressor_free( deflator );
		return 0;
	}

	/* Every call is told the room actually left: tdefl writes no more than
	 * that, and reports a chunk it could not take completely */
	ws_list_iterator_start(liste);
	while ( ( chunk = (html_chunk*) ws_list_iterator_next(liste) ) ) {
		size_t in_bytes = chunk->length;
		size_t out_bytes = capacity - offset;
		status = tdefl_compress( deflator, chunk->text, &in_bytes, &out_buffer[offset], &out_bytes, TDEFL_FULL_FLUSH);
		offset += out_bytes;
		if ( ( status != TDEFL_STATUS_OKAY ) || ( in_bytes != chunk->length ) ){
			printf("deflate: chunk not taken completely, status %s, %lu of %lu bytes\n",
			       get_status(status), offset, capacity);
			failed = 1;
			break;
		}
	}
	ws_list_iterator_stop(liste);

	if ( failed == 0 ){
		size_t in_bytes = 0;
		size_t out_bytes = capacity - offset;
		status = tdefl_compress( deflator, 0, &in_bytes, &out_buffer[offset], &out_bytes, TDEFL_FINISH);
		offset += out_bytes;
		if ( status != TDEFL_STATUS_DONE ){
			printf("deflate: finish failed, status %s, %lu of %lu bytes\n",
			       get_status(status), offset, capacity);
			failed = 1;
		}
	}
	tdefl_compressor_free( deflator );

	return failed ? 0 : offset;
}

void generateOutputBuffer(socket_info* sock) {
	char* buffer;
	uint32_t offset = 0;
	
	
	output_struct *output = WebserverMalloc( sizeof( output_struct ) );
	memset( output, 0 , sizeof( output_struct ) );

	if ( ( sock->use_output_compression == 1 ) && ( sock->send_file_info == 0 ) ){
		unsigned long body_size = getChunkListSize(&sock->html_chunk_list);
		/* Room for the compressed body, which can be larger than the page
		 * itself: deflate does not shrink everything, and every chunk is
		 * flushed on its own. The page size alone, as before, let a page
		 * that compresses badly run over the end of the buffer. */
		unsigned long capacity = deflateCapacity(body_size, ws_list_size(&sock->html_chunk_list));
		buffer = (char*) WebserverMalloc ( capacity + 1  ); /* +1 fuer Header Debug '\0' */

		offset = deflateChunks(&sock->html_chunk_list, buffer, capacity);
		if ( offset > 0 ){
			releaseChunks(&sock->html_chunk_list);
			printHeaderChunk(sock, "Content-Encoding: deflate\r\n");
		}else{
			/* Not compressed after all: the page as it is, the buffer
			 * holds it (capacity > body_size) */
			offset = writeChunksToBuffer(&sock->html_chunk_list, buffer);
		}
		output->body.buffer = buffer;
		output->body.buffer_size = offset;

		if ( body_size == 0){
			printf("url : %s\n",sock->header->url);
		}

		printHeaderChunk(sock, "Content-Length: %"PRIu32"\r\n", offset);
		printHeaderChunk(sock, "\r\n"); /* HTTP Header beenden */

		unsigned long header_size = getChunkListSize(&sock->header_chunk_list);
		buffer = (char*) WebserverMalloc ( header_size + 1  ); /* +1 fuer Header Debug '\0' */

		offset = writeChunksToBuffer(&sock->header_chunk_list, buffer);
		
		output->header.buffer = buffer;
		output->header.buffer_size = offset;
		sock->header->response_bytes = output->header.buffer_size + output->body.buffer_size;
		ws_list_append( &sock->output_list, output, 0 );
		/* timeout_send_idle counts from here, not from the end of the
		 * previous response */
		sock->tx_at = g_loop_now;
		return;
	}
	
	unsigned long header_size = getChunkListSize(&sock->header_chunk_list);
	unsigned long body_size = getChunkListSize(&sock->html_chunk_list);
	
	unsigned long size = header_size + body_size;
	buffer = (char*) WebserverMalloc ( size + 1  ); /* +1 fuer Header Debug '\0' */

	offset = writeChunksToBuffer(&sock->header_chunk_list, buffer);

#ifdef _WEBSERVER_HEADER_DEBUG_
	buffer[offset]='\0';
	LOG (HEADER_PARSER_LOG,NOTICE_LEVEL,sock->socket, "-------- sending Header --------\r\n %s -------- Header end --------",buffer );
#endif

	offset += writeChunksToBuffer(&sock->html_chunk_list, &buffer[offset] );

#ifdef _WEBSERVER_BODY_DEBUG_
	WebServerPrintf ( "%s",buffer );
#endif

#ifdef _WEBSERVER_DEBUG_
	LOG ( CONNECTION_LOG,NOTICE_LEVEL,sock->socket, "compiled HTML Size %ld ",size );
#endif
	output->body.buffer = buffer;
	output->body.buffer_size = offset;
	sock->header->response_bytes = offset;
	
	if ( sock->send_file_info != 0 ){
		sock->header->response_bytes += sock->send_file_info->DataLenght;

		output->file_infos.file_info= sock->send_file_info;
		output->file_infos.file_send_pos = 0;
		/* Released with the output, see releaseOutputFile() */
		output->counted_file = sock->send_file_info;
		output->counted_file->transfers++;

		sock->send_file_info = 0;
	}

	ws_list_append( &sock->output_list, output, 0 );
	/* timeout_send_idle counts from here, not from the end of the previous
	 * response */
	sock->tx_at = g_loop_now;

}

