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

#ifdef _MSC_VER

#include <vld.h> 

/*#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h> 
*/

#include "webserver.h"

#include <math.h>

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


unsigned long VISIBLE_ATTR allocated = 0 ;
unsigned long allocated_max = 0;

#ifdef _WEBSERVER_MEMORY_DEBUG_
    int VISIBLE_ATTR print_blocks_now = 0;
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

void *freeChunkCallBack(const void *free_element) {
	html_chunk* chunk = (html_chunk*)free_element;
	if (chunk->text != 0){
		WebserverFree(chunk->text);
	}
	WebserverFree(chunk);
	return 0;
}

void initMemoryManager(void) {


	PlatformCreateMutex(&mem_mutex);

	ws_list_init(&chunk_cache);

	/* ws_list_attributes_freer(&chunk_cache,freeChunkCallBack); */
#ifdef ENABLE_DEVEL_WARNINGS	
	#warning "chunk cache elemente richtig freigeben"
#endif
}



void freeMemoryManager(void) {
	PlatformLockMutex(&mem_mutex);


	ws_list_destroy(&chunk_cache);
}



#define __BIGGEST_ALIGNMENT__ 16


#ifdef _WEBSERVER_MEMORY_DEBUG_
void* real_WebserverMalloc(const unsigned long size, const char* function, const char* filename, int fileline ) {
#else
void* real_WebserverMalloc(const unsigned long size ) {
#endif
	unsigned long *s;
	unsigned int add_size = 0, real_alloc;
	char* ret;

	add_size += __BIGGEST_ALIGNMENT__ ;

	real_alloc = size + add_size;

	if ( size != 0 ){
		WS_ASSERT( real_alloc == add_size)
	}

	PlatformLockMutex(&mem_mutex);

	ret = (char*) PlatformMalloc(real_alloc);
	if (ret == 0) {
		printf("Memory malloc Error\n");
		exit(1);
	}


	s = (unsigned long*)ret;
	*s = real_alloc;
	ret = ret + __BIGGEST_ALIGNMENT__;

	allocated += real_alloc;
	if (allocated_max < allocated){
		allocated_max = allocated;
	}

	PlatformUnlockMutex(&mem_mutex);

	return (void*) ret;
}

void* WebserverRealloc(void *mem, const unsigned long size ) {
	unsigned long* s;
	char* p;
	char* p2 = mem;
	void* ptr = realloc(p2 - __BIGGEST_ALIGNMENT__,  size  + __BIGGEST_ALIGNMENT__ + sizeof(unsigned long) );
	if ( ptr == 0 ){
		printf("Memory realloc Error\n");
		exit(1);
	}

	p = (char*) ptr;

	s = (unsigned long*)p;


	*s = size + __BIGGEST_ALIGNMENT__ + sizeof(unsigned long);

	p2 = ptr;
	return p2 + __BIGGEST_ALIGNMENT__;


}

static void *get_real_pointer(void* mem){
	char* p;
	if (mem == 0){
		return 0;
	}
	p = (char*) mem;


	p -= __BIGGEST_ALIGNMENT__;

	return p;
}


unsigned long WebserverMallocedSize(void* mem){
	unsigned long* s;
	unsigned long size;
	char* p;

	if (mem == 0){
		return 0;
	}

	p = get_real_pointer( mem );
	s = (unsigned long*) p;


	size = ( *s ) - ( __BIGGEST_ALIGNMENT__ + sizeof(unsigned long) );

	return size;
}



unsigned long WebserverMallocedRealSize(void* mem){
	unsigned long* s;
	unsigned long size;
	char* p;

	if (mem == 0){
		return 0;
	}

	p = get_real_pointer( mem );
	s = (unsigned long*)p;
	size = ( *s );


	return size;
}

void WebserverFree(void *mem) {
	char* p;

	unsigned long size;

	if (mem == 0){
		return;
	}

	PlatformLockMutex(&mem_mutex);


	p = get_real_pointer( mem );

	size = WebserverMallocedRealSize( mem );


	allocated -= size;


	PlatformFree((void*) p);

	PlatformUnlockMutex(&mem_mutex);

}



#endif