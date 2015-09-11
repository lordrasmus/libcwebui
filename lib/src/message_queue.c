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

#include "webserver.h"


#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


ALL_SRC ws_MessageQueue* ws_createMessageQueue(){
	ws_MessageQueue* mq = (ws_MessageQueue*)WebserverMalloc( sizeof(ws_MessageQueue) );
	PlatformCreateMutex(&mq->lock);
	ws_list_init(&mq->entry_list);
	sem_init(&mq->semid, 0, 0);
	return mq;
}


ALL_SRC void *ws_popQueue(ws_MessageQueue* mq){
	void* ret;
	sem_wait(&mq->semid);
	PlatformLockMutex(&mq->lock);
	ret = ws_list_extract_at(&mq->entry_list,0);
	PlatformUnlockMutex(&mq->lock);
	return ret;
}


ALL_SRC void ws_pushQueue(ws_MessageQueue* mq,void* element){
	PlatformLockMutex(&mq->lock);
	ws_list_append(&mq->entry_list,element);
	PlatformUnlockMutex(&mq->lock);
	sem_post(&mq->semid);
}

