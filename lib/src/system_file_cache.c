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

#include "intern/hashmap.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

static hashmap_t* file_cache;

static void fileCacheFree(void *a) {
	WebserverFileInfo* wfi = (WebserverFileInfo*) a;
	if (wfi->Uncompressed != 0){
		/* shares Url, FilePath, etag ... with wfi, owns only its Data */
		WebserverFree( (void*) wfi->Uncompressed->Data);
		WebserverFree(wfi->Uncompressed);
	}
	if (wfi->Deflate != 0){
		/* its Data points into wfi's packed data */
		WebserverFree(wfi->Deflate);
	}
	if (wfi->Url != 0){
		WebserverFree( (void*) wfi->Url);
	}
	if (wfi->FilePath != 0){
		WebserverFree( (void*) wfi->FilePath);
	}
	if (wfi->Data != 0){
		WebserverFree( (void*) wfi->Data);
	}
	if (wfi->lastmodified != 0){
		WebserverFree(wfi->lastmodified);
	}
	if (wfi->etag != 0){
		WebserverFree( (void*) wfi->etag);
	}
	WebserverFree(wfi);
}

void initFileCache(void) {
	file_cache = hashmap_create();
}

void freeFileCache(void) {
	hashmap_destroy(file_cache, fileCacheFree);
}

void addFileToCache(WebserverFileInfo* wfi) {
	hashmap_put(file_cache, wfi->Url, wfi);
}

WebserverFileInfo* getFileFromRBCache(char* name) {
	WebserverFileInfo* wfi;
	wfi = (WebserverFileInfo*) hashmap_get(file_cache, name);
	if (wfi == 0){
#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
		LOG ( FILESYSTEM_LOG,ERROR_LEVEL,0,"Search File %s not found",name);
#endif
		return 0;
	}
#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG ( FILESYSTEM_LOG,ERROR_LEVEL,0,"Search File %s found",name);
#endif
	return wfi;
}

/* Callback context for getLoadedFilesSize */
typedef struct {
	unsigned long filesize;
	int count;
} file_size_ctx_t;

static void calcFileSizeCallback(const char* key, void* value, void* user_data) {
	WebserverFileInfo *wfi = (WebserverFileInfo*) value;
	file_size_ctx_t *ctx = (file_size_ctx_t*) user_data;
	(void)key;

	ctx->filesize += wfi->etagLength;
	ctx->filesize += wfi->lastmodifiedLength;
	if (wfi->RamCached == 1){
		ctx->filesize += wfi->DataLenght;
	}
	ctx->filesize += wfi->FilePathLengt;
	ctx->filesize += sizeof(WebserverFileInfo);
	ctx->count++;
}

unsigned long getLoadedFilesSize(int *p_count) {
	file_size_ctx_t ctx = {0, 0};

	hashmap_foreach(file_cache, calcFileSizeCallback, &ctx);

	if (p_count != 0){
		*p_count = ctx.count;
	}

	return ctx.filesize;
}

static void dumpFileCallback(const char* key, void* value, void* user_data) {
	WebserverFileInfo *wfi = (WebserverFileInfo*) value;
	http_request *s = (http_request*) user_data;
	int filesize;
	const char *fs_type_str;
	const char *compressed_str;
	(void)key;

	/* Calculate file size */
	filesize = wfi->etagLength;
	filesize += wfi->lastmodifiedLength;
	if (wfi->RamCached == 1) {
		filesize += wfi->DataLenght;
	}
	filesize += wfi->FilePathLengt;
	filesize += sizeof(WebserverFileInfo);

	/* Determine filesystem type */
	switch (wfi->fs_type) {
		case FS_BINARY:            fs_type_str = "binary"; break;
		case FS_LOCAL_FILE_SYSTEM: fs_type_str = "local";  break;
		case FS_WNFS:              fs_type_str = "wnfs";   break;
		default:                   fs_type_str = "-";      break;
	}

	/* Determine compression type */
	switch (wfi->Compressed) {
		case 1:  compressed_str = "gzip";    break;
		case 2:  compressed_str = "deflate"; break;
		default: compressed_str = "-";       break;
	}

	printHTMLChunk(s->socket,
		"<tr>"
			"<td class=\"mono\">%s</td>"
			"<td>%d %s</td>"
			"<td>%s</td>"
			"<td>%s</td>"
			"<td>%s</td>"
			"<td>%s</td>"
			"<td>%s</td>"
		"</tr>",
		wfi->Url,
		(filesize > 1024) ? (filesize / 1024) : filesize,
		(filesize > 1024) ? "kB" : "B",
		fs_type_str,
		(wfi->RamCached == 1) ? "yes" : "-",
		(wfi->auth_only == 1) ? "yes" : "-",
		compressed_str,
		(wfi->TemplateFile == 1) ? "yes" : "-");
}

void dumpLoadedFiles(http_request *s) {
	hashmap_foreach(file_cache, dumpFileCallback, s);
}

/* Öffentliche Aufzählung des Datei-Caches (eingebettete Dateien und bereits geladene
 * lokale Dateien), z. B. um eingebettete Sprachdateien zu finden. */
typedef struct {
	void (*cb)(const char* path, void* user);
	void* user;
} foreach_ctx_t;

static void forEachCallback(const char* key, void* value, void* user_data) {
	foreach_ctx_t* ctx = (foreach_ctx_t*) user_data;
	(void)value;
	ctx->cb(key, ctx->user);
}

void VISIBLE_ATTR forEachCachedFile(void (*cb)(const char* path, void* user), void* user) {
	foreach_ctx_t ctx = { cb, user };
	if (file_cache == 0 || cb == 0) return;
	hashmap_foreach(file_cache, forEachCallback, &ctx);
}
