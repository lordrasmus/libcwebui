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


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


ws_MessageQueue* ws_createMessageQueue( void ){
	ws_MessageQueue* mq = (ws_MessageQueue*)WebserverMalloc( sizeof(ws_MessageQueue) );
	PlatformCreateMutex(&mq->lock);
	ws_list_init(&mq->entry_list);
	PlatformCreateSem(&mq->semid, 0);
	return mq;
}


void *ws_popQueue(ws_MessageQueue* mq){
	void* ret;
	PlatformWaitSem(&mq->semid);
	PlatformLockMutex(&mq->lock);
	ret = ws_list_extract_at(&mq->entry_list,0);
	PlatformUnlockMutex(&mq->lock);
	return ret;
}


void ws_pushQueue(ws_MessageQueue* mq,void* element){
	PlatformLockMutex(&mq->lock);
	ws_list_append(&mq->entry_list,element);
	PlatformUnlockMutex(&mq->lock);
	PlatformPostSem(&mq->semid);
}

