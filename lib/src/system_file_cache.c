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

#include "webserver.h"

#include "red_black_tree.h"

rb_red_blk_tree* file_cache;

void FileDest( UNUSED_PARA void* a) {
	//free((int*) a);
}

int FileComp(const void* a, const void* b) {
	int ret;
	char *p_a = (char*) a;
	char *p_b = (char*) b;
	ret = strcmp(p_a, p_b);
	if (ret < 0) return -1;
	if (ret > 0) return 1;
	return 0;
}

void FilePrint(const void* a) {
	printf("%i", *(int*) a);
}

void InfoPrint( UNUSED_PARA void* a) {
}

void InfoDest(void *a) {
	WebserverFileInfo* wfi = (WebserverFileInfo*) a;
	if (wfi->Url != 0) WebserverFree(wfi->Url);
	if (wfi->FilePath != 0) WebserverFree(wfi->FilePath);
	if (wfi->Data != 0) WebserverFree(wfi->Data);
	if (wfi->lastmodified != 0) WebserverFree(wfi->lastmodified);
	if (wfi->etag != 0) WebserverFree(wfi->etag);
	WebserverFree(wfi);
}

void initFileCache(void) {
	file_cache = RBTreeCreate(FileComp, FileDest, InfoDest, FilePrint, InfoPrint);
}

void freeFileCache(void) {
	RBTreeDestroy(file_cache);
}

void addFileToCache(WebserverFileInfo* wfi) {
	RBTreeInsert(file_cache, wfi->Url, wfi);
}

WebserverFileInfo* getFileFromRBCache(char* name) {
	WebserverFileInfo* wfi;
	rb_red_blk_node* node = RBExactQuery(file_cache, name);
	if (node == 0){
#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
		LOG ( FILESYSTEM_LOG,ERROR_LEVEL,0,"Search File %s not found",name);
#endif
		return 0;
	}
	wfi = (WebserverFileInfo*) (node)->info;
#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG ( FILESYSTEM_LOG,ERROR_LEVEL,0,"Search File %s found",name);
#endif
	return wfi;
}

unsigned long getLoadedFilesSize(int *p_count) {
	unsigned long filesize = 0;
	WebserverFileInfo *wfi;
	stk_stack* stack;
	rb_red_blk_node* node;
	int count = 0;

	stack = RBEnumerate(file_cache, (void*)"0", (void*)"z");

	while (0 != StackNotEmpty(stack)) {
		node = (rb_red_blk_node*) StackPop(stack);
		wfi = (WebserverFileInfo*) node->info;
		filesize += wfi->etagLength;
		filesize += wfi->lastmodifiedLength;
		if (wfi->RamCached == 1) filesize += wfi->DataLenght;
		filesize += wfi->FilePathLengt;
		filesize += sizeof(WebserverFileInfo);
		filesize += sizeof(rb_red_blk_node);
		count++;
	}
	if (p_count != 0) *p_count = count;

	free(stack);

	return filesize;
}

void dumpLoadedFiles(http_request *s) {
	int filesize = 0;
	WebserverFileInfo *wfi;
	stk_stack* stack;
	rb_red_blk_node* node;

	stack = RBEnumerate(file_cache, (void*)"0", (void*)"z");

	while (0 != StackNotEmpty(stack)) {
		node = (rb_red_blk_node*) StackPop(stack);
		wfi = (WebserverFileInfo*) node->info;
		filesize = wfi->etagLength;
		filesize += wfi->lastmodifiedLength;
		if (wfi->RamCached == 1) filesize += wfi->DataLenght;
		filesize += wfi->FilePathLengt;
		filesize += sizeof(WebserverFileInfo);
		printHTMLChunk(s->socket, "<tr><td>%s<td>%d<td>%d", wfi->Url, filesize, wfi->RamCached);
	}
	free(stack);
}

