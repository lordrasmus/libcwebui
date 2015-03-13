/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WS_LINKED_LIST_H_
#define _WS_LINKED_LIST_H_


#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif 



typedef void *(*element_freer)(const void *free_element);
typedef int (*element_seeker)(const void *el, const void *indicator);


struct list_entry{
    struct list_entry* next;
    struct list_entry* prev;
    
    void* data;
};

typedef struct {
    
    unsigned int element_count;
    struct list_entry* first;
    
    struct list_entry* cur_iter;
    
    element_seeker seeker;
    element_freer freer;
    
} list_t;

int ws_list_init(list_t *l);
void ws_list_destroy(list_t *l);

int ws_list_attributes_freer(list_t *l, element_freer freer_fun);
int ws_list_clear(list_t *l);


int ws_list_iterator_start(list_t *l);
void *ws_list_iterator_next(list_t *l);
int ws_list_iterator_hasnext(const list_t *l);
int ws_list_iterator_stop(list_t *l);

int ws_list_append(list_t *l, void *data);
int ws_list_delete(list_t *l, void *data);

void *ws_list_extract_at(list_t *l, unsigned int pos);
void *ws_list_get_at(const list_t *l, unsigned int pos);

void *ws_list_seek(list_t *l, const void *indicator);
int ws_list_attributes_seeker(list_t *l, element_seeker seeker_fun);

unsigned int ws_list_size(const list_t *l);

int ws_list_empty(const list_t *l);

#ifdef __cplusplus
}
#endif

#endif
