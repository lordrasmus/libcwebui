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


/* unter win64 ist der node* der zurckgegeben wird nur 32 bit lang ?? */

#ifndef USE_SIMCLIST

#ifdef ENABLE_OLD_LIST

ListNode* getNewListNode(void){
	ListNode* ret = MallocListNode();
	ret->next=0;
	ret->prev=0;
	ret->value=0;
	return ret;
}

ListNode* addNewListNode(ListNode* list,void* value){
	ListNode *newLN;
	newLN = getNewListNode();
	newLN->value = value;

	if(list == 0){		
		return newLN;
	}else{
		if(list->next != 0){
			newLN->next = list->next;
			((ListNode*)list->next)->prev=newLN;		
		}		
		list->next = newLN;
		newLN->prev = list;
		return newLN;
	}
}

ListNode* addNewListNodeEnd(ListNode* list,void* value){
	ListNode *newLN,*cur;
	if(list == 0){
		newLN = getNewListNode();
		newLN->value = value;
		return newLN;
	}else{
		newLN = getNewListNode();
		newLN->value = value;
		cur = list;
		while(1){
			if(cur->next==0)
				break;
			cur = (ListNode*)cur->next;
		}
				
		cur->next = newLN;
		newLN->prev = cur;
		return newLN;
	}
}

ListNode* getFirstListNode(ListNode* list){
	ListNode* cur = list;
	if(list == 0) return 0;
	while(cur->prev != 0){
		cur = (ListNode*)cur->prev;
	}
	return cur;
}

ListNode* getNextListNode(ListNode* list){
	if(list == 0) return 0;
	return (ListNode*)list->next;
}

ListNode* removeListNode(ListNode* node){
	//ListNode* next=0;
	//ListNode* prev=0;
	if(node == 0) return 0;

	if(node->next != 0){
		if(node->prev != 0){
			((ListNode*)node->prev)->next = node->next;
			((ListNode*)node->next)->prev = node->prev;
			return (ListNode*)node->next;			
		}else{
			((ListNode*)node->next)->prev = 0;
			return (ListNode*)node->next;
		}
	}else{
		if(node->prev != 0){
			((ListNode*)node->prev)->next = 0;			
			return (ListNode*)node->prev;			
		}else{
			return 0;
		}
	}
}

ListNode* deleteListNode(ListNode* node,void (*fp)(void*)){
	ListNode* ret;
	if(node == 0) return 0;
	ret = removeListNode(node);
	if(node->value != 0){
		if(fp != 0)
			fp(node->value);
	}
	//WebserverFree(node);
	FreeListNode(node);
	
	return ret;
}


ListNode* deleteList(ListNode* node,void (*fp)(void*)){
	ListNode* list;//,*del;

	list = getFirstListNode(node);
	while(list != 0){		
		list = deleteListNode(list,fp);
	}
	return 0;
}

#endif

#endif
