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
*

	Hat was mit nem sig pipe zu tun

Webserver: handleWebsocket : src/websocket_handler.c 90                ERROR   : ( 19 ) : Darf hier niemals ankommen  status: 4  send_bytes: 0  to_send: 336
Webserver: WebserverSLLSendNonBlocking : ssl/openssl.c 478             ERROR   : ( 19 ) : Unhandled SSL Error ( 5 ) error:00000000:lib(0):func(0):reason(0)
Webserver: handleWebsocket : src/websocket_handler.c 90                ERROR   : ( 19 ) : Darf hier niemals ankommen  status: 4  send_bytes: 0  to_send: 336
Webserver: WebserverSLLSendNonBlocking : ssl/openssl.c 478             ERROR   : ( 19 ) : Unhandled SSL Error ( 5 ) error:00000000:lib(0):func(0):reason(0)
Webserver: handleWebsocket : src/websocket_handler.c 90                ERROR   : ( 19 ) : Darf hier niemals ankommen  status: 4  send_bytes: 0  to_send: 336
Webserver: WebserverSLLSendNonBlocking : ssl/openssl.c 478             ERROR   : ( 19 ) : Unhandled SSL Error ( 5 ) error:00000000:lib(0):func(0):reason(0)


*/

#include "stdafx.h"

#ifdef __GNUC__
#include "webserver.h"
#include "simclist.h"
#include <math.h>
#endif

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


unsigned long allocated = 0;
unsigned long allocated_max = 0;

#ifdef _WEBSERVER_MEMORY_DEBUG_
    int print_blocks_now = 0;
#endif


/*
#define USE_MALLOC_CACHE
*/

#ifdef _WEBSERVER_MEMORY_DEBUG_
list_t block_list;
#endif

list_t chunk_cache;

#ifdef USE_MALLOC_CACHE
list_t malloc_cache[20];
#warning "Auf uClibc gibt es assertions"
#endif

WS_MUTEX mem_mutex;

void *freeChunkCallBack(const void *restrict free_element) {
	html_chunk* chunk = (html_chunk*)free_element;
	if (chunk->text != 0) WebserverFree(chunk->text);
	WebserverFree(chunk);
	return 0;
}

ALL_SRC void initMemoryManager(void) {

#ifdef USE_MALLOC_CACHE
	int i;
#endif

	PlatformCreateMutex(&mem_mutex);

#ifdef _WEBSERVER_MEMORY_DEBUG_
	ws_list_init(&block_list);
#endif

#ifdef USE_MALLOC_CACHE
	for (i = 0; i < 20; i++)
	ws_list_init(&malloc_cache[i]);
#endif

	ws_list_init(&chunk_cache);

	/* ws_list_attributes_freer(&chunk_cache,freeChunkCallBack); */
#warning "chunk cache elemente richtig freigeben"
}



void freeMemoryManager(void) {
	PlatformLockMutex(&mem_mutex);

#ifdef USE_MALLOC_CACHE
	for (i = 0; i < 20; i++)
	ws_list_destroy(&malloc_cache[i]);
#endif

	ws_list_destroy(&chunk_cache);
}

#ifdef _WEBSERVER_MEMORY_DEBUG_

typedef struct {
	char* filename;
	unsigned int fileline;
	unsigned int size;
	unsigned int real_size;
	void* real_pointer;
	void* pointer;
}memory_block;



void addBlock(memory_block* block) {
	ws_list_append(&block_list, block);
}

void delBlock(void* p_block) {
	memory_block* block = (memory_block*) p_block;
	ws_list_delete(&block_list, block);
}

void printBlocks(void) {
	memory_block* block;
	unsigned long blocks = 0;
	unsigned long size = 0;
	int i = 0;
	char buffer[100];
	FILE* file;

	while(1){
		sprintf(buffer,"/tmp/webserver_memory_%d.txt",i);
		if ( 0 != access(buffer, R_OK)){
			break;
		}
		i++;
	}

	file = fopen(buffer, "w");
	fprintf(file,"Start Speicher Debug\n");

	PlatformLockMutex(&mem_mutex);

	ws_list_iterator_start(&block_list);
	while ((block = (memory_block*) ws_list_iterator_next(&block_list))) {
		blocks++;
		//size += WebserverMallocedRealSize(block->pointer);
		size += block->real_size;
		fprintf(file,"Size : %d File :%s  Line :%d \n", block->size, block->filename, block->fileline);
	}
	ws_list_iterator_stop(&block_list);

	fprintf(file,"Ende Speicher Debug Blocks: %ld Size : %ld ( %ld )\n", blocks,size,allocated);

	PlatformUnlockMutex(&mem_mutex);

	fclose( file );
}

#endif

#ifdef USE_MALLOC_CACHE

#ifdef __GNUC__
#define clzl(x)  __builtin_clzl(x)
#ifdef __LP64__
#define LONG_BITS 63
#pragma message("64Bit")
#endif
#if ULONG_MAX == 0xFFFFFFFFL
#define LONG_BITS 31
#pragma message("32Bit")
#endif
#endif

unsigned int clz(unsigned long size) {
#ifdef _MSC_VER
	int r = 0;
	unsigned long tmp = 1 << r;
	while ( tmp < size ) {
		r++;
		tmp = 1 << r;
	}
	return r;
#endif

#ifdef __GNUC__
	return LONG_BITS - clzl(size);
#endif
}

#define MAX_BLOCK_SIZE_INDEX 15	// max 16384 byte blocks cachen
#define PREE_ALOCATE_BLOCKS 5

unsigned long calc_malloc_index(unsigned long size) {
	unsigned int r;
	unsigned long m;
	r = clz(size);
	m = 1 << r;
	if (m < size) r++;
	m = 1 << r;
	return r;
}

void* get_cached_malloc(unsigned long size) {
	void *ret = 0;
	unsigned long m;
	unsigned long malloc_size = calc_malloc_index(size);
	int i;

	if (malloc_size < MAX_BLOCK_SIZE_INDEX) {
		if (malloc_cache[malloc_size].numels > 0) {
			//m = 1 << malloc_size;
			ret = ws_list_extract_at(&malloc_cache[malloc_size], 0);
		} else {
			m = 1 << malloc_size;
			for(i=0;i<PREE_ALOCATE_BLOCKS;i++) {
				ret = PlatformMalloc(m);
				ws_list_append(&malloc_cache[malloc_size], ret);
			}
			ret = PlatformMalloc(m);
		}
	}
	if (ret == 0) {
		m = 1 << malloc_size;
		ret = PlatformMalloc(m);

	}
	return ret;
}

void insert_cached_malloc(void* mem, unsigned long size) {
	//unsigned long m;
	unsigned int n = calc_malloc_index(size);

	if (n < MAX_BLOCK_SIZE_INDEX) {
		//m = 1 << n;
		ws_list_append(&malloc_cache[n], mem);
	} else {
		PlatformFree(mem);
	}
}

#endif

#pragma GCC visibility push(default)

#ifndef __BIGGEST_ALIGNMENT__
	#define __BIGGEST_ALIGNMENT__ 8
#endif

#if __BIGGEST_ALIGNMENT__ < __SIZEOF_LONG__
	#define MEM_OFFSET __SIZEOF_LONG__
#else
	#define MEM_OFFSET __BIGGEST_ALIGNMENT__
#endif

#ifdef _WEBSERVER_MEMORY_DEBUG_
void* real_WebserverMalloc(const unsigned long size, const char* function, const char* filename, int fileline ) {
#else
void* real_WebserverMalloc(const unsigned long size ) {
#endif
	unsigned long *s;
	unsigned int add_size = 0, real_alloc;
	char* ret;
	//char* ret2;
#ifdef _WEBSERVER_MEMORY_DEBUG_
	memory_block* block;
	add_size += sizeof(memory_block);
	add_size += __BIGGEST_ALIGNMENT__ * 4;
#endif

	add_size += __BIGGEST_ALIGNMENT__ + sizeof(unsigned long);

	real_alloc = size + add_size;

	if ( size != 0 ){
		WS_ASSERT( real_alloc == add_size)
	}

	PlatformLockMutex(&mem_mutex);

#ifdef USE_MALLOC_CACHE
	ret = (char*)get_cached_malloc(real_alloc);
#else
	ret = (char*) PlatformMalloc(real_alloc);
#endif
	if (ret == 0) {
		printf("Memory malloc Error\n");
		exit(1);
	}
	//ret2 = ret;
#if __BIGGEST_ALIGNMENT__ == 16
	s = (unsigned long*) __ws_assume_aligned(ret, 8 );
	*s = real_alloc;
	ret = __ws_assume_aligned(ret + MEM_OFFSET, 8 );
#else
	s = (unsigned long*) __ws_assume_aligned(ret, __BIGGEST_ALIGNMENT__ );
	*s = real_alloc;
	ret = __ws_assume_aligned(ret + MEM_OFFSET, __BIGGEST_ALIGNMENT__ );
#endif

	allocated += real_alloc;
	if (allocated_max < allocated) allocated_max = allocated;


#ifdef _WEBSERVER_MEMORY_DEBUG_
	block = (memory_block*) __ws_assume_aligned(ret,__BIGGEST_ALIGNMENT__);
	block->filename = (char*) filename;
	block->fileline = fileline;
	block->size = size;
	block->real_size = real_alloc;
	block->real_pointer = ret2;

	addBlock(block);

	ret += sizeof(memory_block);
	memset(ret , 0xAB, __BIGGEST_ALIGNMENT__ * 2);
	ret += __BIGGEST_ALIGNMENT__ * 2;
	memset(ret + size, 0xAB, __BIGGEST_ALIGNMENT__ * 2);

	block->pointer = ret;
#endif

	PlatformUnlockMutex(&mem_mutex);

	return (void*) ret;
}

void* WebserverRealloc(void *mem, const unsigned long size ) {
	unsigned long* s;
	char* p;
	void* ptr = realloc(mem - MEM_OFFSET,  size  + __BIGGEST_ALIGNMENT__ + sizeof(unsigned long) );
	if ( ptr == 0 ){
		printf("Memory realloc Error\n");
		exit(1);
	}

	p = (char*) ptr;

#if __BIGGEST_ALIGNMENT__ == 16
	s = (unsigned long*) __ws_assume_aligned(p, 8 );
#else
	s = (unsigned long*) __ws_assume_aligned(p, __BIGGEST_ALIGNMENT__ );
#endif

	*s = size + __BIGGEST_ALIGNMENT__ + sizeof(unsigned long);

#if __BIGGEST_ALIGNMENT__ == 16
	return __ws_assume_aligned(ptr + MEM_OFFSET, 8 );
#else
	return __ws_assume_aligned(ptr + MEM_OFFSET, __BIGGEST_ALIGNMENT__ );
#endif

	#warning "an Memory Debug anpassen"
}

static void *get_real_pointer(void* mem){
	char* p;
	if (mem == 0) return 0;
	p = (char*) mem;

#ifdef _WEBSERVER_MEMORY_DEBUG_

	p -= __BIGGEST_ALIGNMENT__ * 2;
	p -= sizeof(memory_block);

#endif

	p -= MEM_OFFSET;

	return p;
}

#ifdef _WEBSERVER_MEMORY_DEBUG_

static void *get_block_pointer(void* mem){
	char* p;
	if (mem == 0) return 0;
	p = (char*) mem;

	p -= __BIGGEST_ALIGNMENT__ * 2;
	p -= sizeof(memory_block);

	return p;
}

#endif

unsigned long WebserverMallocedSize(void* mem){
	unsigned long* s;
	unsigned long size;
	char* p;

	if (mem == 0) return 0;

#ifdef _WEBSERVER_MEMORY_DEBUG_

	memory_block* block = (memory_block*) __ws_assume_aligned(get_block_pointer(mem), __BIGGEST_ALIGNMENT__ );
	block = get_block_pointer( mem );

#endif

	p = get_real_pointer( mem );
#if __BIGGEST_ALIGNMENT__ == 16
	s = (unsigned long*) __ws_assume_aligned(p, 8 );
#else
	s = (unsigned long*) __ws_assume_aligned(p, __BIGGEST_ALIGNMENT__ );
#endif

	size = ( *s ) - ( __BIGGEST_ALIGNMENT__ + sizeof(unsigned long) );

#ifdef _WEBSERVER_MEMORY_DEBUG_
	size -= __BIGGEST_ALIGNMENT__ * 4;
	size -= sizeof(memory_block);

	if ( size != block->size){
		printf("Error Verifieng Memory size\n");
	}

#endif

	return size;
}



unsigned long WebserverMallocedRealSize(void* mem){
	unsigned long* s;
	unsigned long size;
	char* p;

	if (mem == 0) return 0;
	p = (char*) mem;

#ifdef _WEBSERVER_MEMORY_DEBUG_

	p -= __BIGGEST_ALIGNMENT__ * 2;
	p -= sizeof(memory_block);

#endif

	p -= MEM_OFFSET;

	p = get_real_pointer( mem );
#if __BIGGEST_ALIGNMENT__ == 16
	s = (unsigned long*) __ws_assume_aligned(p, 8 );
#else
	s = (unsigned long*) __ws_assume_aligned(p, __BIGGEST_ALIGNMENT__ );
#endif
	size = ( *s );


	return size;
}

void WebserverFree(void *mem) {
	char* p;
#ifdef _WEBSERVER_MEMORY_DEBUG_
	int  i;
	unsigned char *check;
	memory_block* block;
#endif

	unsigned long size;

	if (mem == 0) return;

	PlatformLockMutex(&mem_mutex);

#ifdef _WEBSERVER_MEMORY_DEBUG_

	block = (memory_block*) __ws_assume_aligned(get_block_pointer(mem), __BIGGEST_ALIGNMENT__ );
	delBlock(block);

#endif

	p = get_real_pointer( mem );

	size = WebserverMallocedRealSize( mem );

#ifdef _WEBSERVER_MEMORY_DEBUG_
	if( size != block->real_size ){
		printf("Memory Size Info Error %ld %d\n",size, block->real_size );
	}

	if ( p != block->real_pointer ){
		printf("Memory Pointer Error 0x%lX 0x%lX\n",(unsigned long)p, (unsigned long)block->pointer );
	}
#endif

	allocated -= size;


#ifdef _WEBSERVER_MEMORY_DEBUG_
	check = ((unsigned char*)block) + sizeof(memory_block);
	for (i = 0; i < __BIGGEST_ALIGNMENT__ * 2 ; i++) {
		if (check[i] != 0xAB) {
			printf("Memory Start Fence Error : Offset %d ",i);
			printf("Size : %d File :%s  Line :%d \n", block->size, block->filename, block->fileline);
			break;
		}
	}

	check = ((unsigned char*)mem) + WebserverMallocedSize(mem);
	for (i = 0; i < __BIGGEST_ALIGNMENT__ * 2 ; i++) {
		if (check[i] != 0xAB) {
			printf("Memory End Fence Error : Offset %d ",i);
			printf("Size : %d File :%s  Line :%d \n", block->size, block->filename, block->fileline);
			break;
		}
	}

#endif

#ifdef USE_MALLOC_CACHE
	insert_cached_malloc((void*) p, *s);
#else
	PlatformFree((void*) p);
#endif


	PlatformUnlockMutex(&mem_mutex);

}

#pragma GCC visibility pop

void WebserverFreeMem(void) {
	/*    for (i=0;i<g_files.FileCount;i++)
	 {
	 WebserverFree(g_files.files[i]->Name);
	 WebserverFree(g_files.files[i]->Data);
	 WebserverFree(g_files.files[i]);
	 }
	 WebserverFree(g_files.files);*/
	/* TODO: "freigeben der datai infos wieder einbauen" */
}

Parameter* WebserverMallocParameter(void) {
	Parameter* ret = (Parameter*) WebserverMalloc( sizeof(Parameter) );
	ret->name = 0;
	ret->value = 0;
	return ret;
}

void WebserverFreeParameter(Parameter* para) {
	if (para->name != 0) WebserverFree(para->name);
	if (para->value != 0) WebserverFree(para->value);
	WebserverFree(para);
}

socket_info* WebserverMallocSocketInfo(void) {
	socket_info* sock = (socket_info*) WebserverMalloc( sizeof(socket_info) );
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

void freeChunkList(list_t* liste) {
	html_chunk* chunk;
	ws_list_iterator_start(liste);
	while ((chunk = (html_chunk*) ws_list_iterator_next(liste))) {
		WebserverFreeHtml_chunk(chunk);
	}
	ws_list_iterator_stop(liste);
	ws_list_destroy(liste);
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
#endif

	freeChunkList(&sock->header_chunk_list);
	freeChunkList(&sock->html_chunk_list);


	#warning "ssl strukturen freigeben"

	ws_list_destroy(&sock->firephplogs);

	PlatformDestroyMutex(&sock->socket_mutex);

	WebserverFree(sock);


}

Cookie* WebserverMallocCookie(void) {
	Cookie* ret = (Cookie*) WebserverMalloc( sizeof(Cookie) );
	memset(ret, 0, sizeof(Cookie));
	return ret;
}

void WebserverFreeCookie(Cookie* cookie) {
	if ( cookie == 0){
		return;
	}
	if (cookie->name != 0){
		WebserverFree(cookie->name);
	}
	if (cookie->value != 0){
		WebserverFree(cookie->value);
	}
	WebserverFree(cookie);
}

ws_variable* WebserverMallocVariable_store(void) {
	ws_variable* ret = (ws_variable*) WebserverMalloc( sizeof(ws_variable) );
	memset(ret, 0, sizeof(ws_variable));
	return ret;
}

void WebserverFreeVariable_store(ws_variable* store) {
	WebserverFree(store);
}

sessionStore* WebserverMallocSessionStore(void) {
	sessionStore* ret = (sessionStore*) WebserverMalloc( sizeof(sessionStore) );
	memset(ret, 0, sizeof(sessionStore));
	ret->vars = createVariableStore();
	return ret;
}

void WebserverFreeSessionStore(sessionStore* store) {
	deleteVariableStore(store->vars);
	WebserverFree(store);
}

void *WebserverFreeCookieFreer(const void *restrict free_element) {
	WebserverFreeCookie((Cookie*) free_element);
	return 0;
}

HttpRequestHeader* WebserverMallocHttpRequestHeader(void) {
	HttpRequestHeader* header = (HttpRequestHeader*) WebserverMalloc( sizeof(HttpRequestHeader) );
	memset(header, 0, sizeof(HttpRequestHeader));
	header->parameter_store = createVariableStore();
	ws_list_init(&header->cookie_list);
	ws_list_attributes_freer(&header->cookie_list, WebserverFreeCookieFreer);
	return header;
}

void WebserverResetHttpRequestHeader(HttpRequestHeader *header) {

	clearVariables(header->parameter_store);
	ws_list_clear(&header->cookie_list);


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
	ret = (FUNCTION_PARAS*) WebserverMalloc ( sizeof ( FUNCTION_PARAS ) );
	memset(ret, 0, sizeof(FUNCTION_PARAS));
	return ret;
}

void WebserverFreeFunctionParas(FUNCTION_PARAS *func) {
	WebserverFree(func);
}

html_chunk* WebserverMallocHtml_chunk(void) {
	html_chunk* ret;
	if (chunk_cache.numels == 0) {
		ret = (html_chunk*) WebserverMalloc( sizeof(html_chunk) );
		ret->text = (char*) WebserverMalloc( 2000 );
	} else {
		ret = (html_chunk*) ws_list_extract_at(&chunk_cache, 0);
	}
	ret->length = 0;
	return ret;
}

void WebserverFreeHtml_chunk(html_chunk* chunk) {
	/*if (chunk_cache.numels < 20) {
		ws_list_append(&chunk_cache, chunk);
	} else {*/
		if (chunk->text != 0) WebserverFree(chunk->text);
		WebserverFree(chunk);
	/*}*/
}

