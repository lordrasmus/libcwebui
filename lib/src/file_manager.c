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

#define LM1 (char*)s->header->If_Modified_Since
#define LM2 (char*)info->lastmodified

#define ET1 (char*)s->header->etag
#define ET2 (char*)info->etag

int checkCacheHeader(http_request* s, WebserverFileInfo *info) {
	if ((s == 0) || (s->header == 0) || (info == 0)) {
		return -1;
	}
#ifndef WEBSERVER_DISABLE_CACHE
	if (info->FileType != FILE_TYPE_HTML) {
		if (likely( s->header->etag != 0 )) {
			/*
				ETag Header vorhanden
			*/
			if ((0 == strcmp(ET1, ET2))) {
#ifdef _WEBSERVER_CACHE_DEBUG_
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Cache Hit ETag %s",info->Name );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Request %s",s->header->etag );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "File    %s",info->etag );
#endif
				return 1;
			} else {
#ifdef _WEBSERVER_CACHE_DEBUG_
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Cache Miss ETag %s",info->Name );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Request %s",s->header->etag );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket,"File    %s",info->etag );
#endif
			}
		} else if ((s->header->If_Modified_Since != 0)
				&& (info->lastmodified != 0)) {
			/*
					Modified_Since Header vorhanden
			*/
			if ((0 == strcmp(LM1, LM2)) && (strlen(LM1) == strlen(LM2))) {
#ifdef _WEBSERVER_CACHE_DEBUG_
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket,"Cache Hit Last Modified %s",info->Name );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Request %s",s->header->If_Modified_Since );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "File    %s",info->lastmodified );
#endif
				return 1;
			} else {
#ifdef _WEBSERVER_CACHE_DEBUG_
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket,"Cache Hit Last Modified %s",info->Name );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Request %s",s->header->If_Modified_Since );
				LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "File    %s",info->lastmodified );
#endif
			}
		} else {
#ifdef _WEBSERVER_CACHE_DEBUG_
			LOG(CACHE_LOG,NOTICE_LEVEL,s->socket->socket, "Cache Miss %s no Cache Headers",info->Name );
#endif
		}
	}
#endif

	return 0;

}

int sendFile(http_request* s, WebserverFileInfo *info) {
	if ((s == 0) || (s->socket == 0) || (info == 0)) {
		return -1;
	}
	s->socket->file_infos.file_info = 0;

	if (likely( 1 == checkCacheHeader(s,info) )) {
		sendHeaderNotModified(s, info);
		return 0;
	}

	if (0 > sendHeader(s, info, info->DataLenght))
		return -1;

	s->socket->file_infos.file_send_pos = 0;
	s->socket->file_infos.file_info = info;

	return 0;
}
