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

#ifndef _LIST_H_
#define _LIST_H_




//#define ENABLE_OLD_LIST

#ifdef ENABLE_OLD_LIST

typedef struct
{
    void* prev;
    void* next;
    void* value;
} ListNode;

#ifdef __cplusplus
extern "C" {
#endif

ListNode* addNewListNode(ListNode* list,void* value);
ListNode* addNewListNodeEnd(ListNode* list,void* value);

ListNode* getFirstListNode(ListNode* list);
ListNode* getNextListNode(ListNode* list);

ListNode* removeListNode(ListNode* node);
ListNode* deleteListNode(ListNode* node,void (*fp)(void*));
ListNode* deleteList(ListNode* node,void (*fp)(void*));


#ifdef __cplusplus
}
#endif

#endif

#endif


