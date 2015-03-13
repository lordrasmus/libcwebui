/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#include "intern/linked_list.h"
#include "intern/helper.h"

int VISIBLE_ATTR ws_list_init(list_t *l){
    memset( l, 0 , sizeof( list_t ));
    return 1;
}

void VISIBLE_ATTR ws_list_destroy(list_t *l){
    struct list_entry* cur = l->first;
    struct list_entry* del;
    
    if ( l->first == 0 ){
        return;
    }
     
    while( cur->next != 0 ){
        del = cur;
        cur = cur->next;
        
        if( l->freer != 0 ){
            l->freer( del->data );
        }
        free( del );
        
        l->element_count--;
    }
    
    if( l->freer != 0 ){
        l->freer( cur->data );
    }
    free( cur );
    
    l->element_count--;
    
    l->first = 0;
}

int VISIBLE_ATTR ws_list_clear(list_t *l){
    struct list_entry* cur = l->first;
    struct list_entry* del;
    
    if ( l->first == 0 ){
        return 0;
    }
     
    while( cur->next != 0 ){
        del = cur;
        cur = cur->next;
        
        free( del );
        
        l->element_count--;
    }
    
    free( cur );
    
    l->element_count--;
    
    l->first = 0;
    
    return 1;
}

int VISIBLE_ATTR ws_list_delete(list_t *l, void *data){
    struct list_entry* cur = l->first;
    
    struct list_entry* del = 0;
    
    if ( cur == 0 ){
        return 0;
    }
    
    while( cur->next != 0 ){
        if ( cur->data == data ){
            del = cur;
            break;
        }
        cur = cur->next;
    }    
    
    if ( ( del == 0 ) && ( cur->data == data ) ){
        del = cur;
    }
    
    if ( del == 0 ){
        return 0;
    }
    
    l->element_count--;
    
    // Element gefunden, jetzt löschen
    
    struct list_entry *prev = del->prev;
    struct list_entry *next = del->next;
    
    if( prev ){
        prev->next = next;
    }else{
        if( cur == l->first ){
            l->first = next;
        }
    }
    
    if( next ){
        next->prev = prev;
    }
    
    //if ( l->freer != 0 ) // data wird im original nicht gelöscht
    //    l->freer( del->data );
    
    free( del );
    
    return 1;
}



int VISIBLE_ATTR ws_list_attributes_freer(list_t *l, element_freer freer_fun){
    l->freer = freer_fun;
    return 1;
}

int VISIBLE_ATTR ws_list_attributes_seeker(list_t *l, element_seeker seeker_fun){
    l->seeker = seeker_fun;
    return 1;
}



int VISIBLE_ATTR ws_list_append(list_t *l, void *data){
    
    l->element_count++;
    
    if ( l->first == 0 ){
        struct list_entry* entry = malloc( sizeof( struct list_entry ) );
        memset( entry, 0 , sizeof( struct list_entry ) );
        entry->data = data;
        l->first = entry;
        return -1;
    }
    
    struct list_entry* cur = l->first;
    while( cur->next != 0 ){
        cur = cur->next;
    }
    
    struct list_entry* new_ele = malloc( sizeof( struct list_entry ) );
    memset( new_ele , 0 , sizeof( struct list_entry ) );
    
    cur->next = new_ele;
    new_ele->prev = cur; 
    new_ele->data = data;
    
    return 1;
        
}


int VISIBLE_ATTR ws_list_iterator_start(list_t *l){
    l->cur_iter = l->first;
    return 1;
}

int VISIBLE_ATTR ws_list_iterator_hasnext(const list_t *l){
    if ( l->cur_iter == 0 ){
        return 0;
    }
    return 1;
}

void VISIBLE_ATTR *ws_list_iterator_next(list_t *l){
    struct list_entry* ret =  l->cur_iter;
    if ( ret == 0 ){
        return 0;
    }
    
    l->cur_iter =  l->cur_iter->next;
    return ret->data;
}

int VISIBLE_ATTR ws_list_iterator_stop(list_t *l){
    l->cur_iter = 0;
    return 1;
}

static struct list_entry *ws_list_get_entry_at(const list_t *l, unsigned int pos){
    unsigned int cur_pos = 0;
    if ( l->first == 0 ){
        return 0;
    }
    
    struct list_entry* cur = l->first;
    while( cur->next != 0 ){
        if ( cur_pos++ == pos ){
            return cur;
        }
        cur = cur->next;
    }
    
    if ( cur_pos == pos ){
        return cur;
    }
    
    return 0;
}

void VISIBLE_ATTR *ws_list_extract_at(list_t *l, unsigned int pos){
    struct list_entry* cur = ws_list_get_entry_at( l, pos );
    if ( cur == 0 ){
        return 0;
    }
    
    l->element_count--;
    
    struct list_entry *prev = cur->prev;
    struct list_entry *next = cur->next;
    
    void* ret = cur->data;
    
    if( prev ){
        prev->next = next;
    }else{
        if( cur == l->first ){
            l->first = next;
        }
    }
    
    if( next ){
        next->prev = prev;
    }
    
    free( cur );
    
    return ret;
}


void VISIBLE_ATTR *ws_list_get_at(const list_t *l, unsigned int pos){
    
    struct list_entry* cur = ws_list_get_entry_at( l, pos );
    if ( cur == 0 ){
        return 0;
    }
    
    return cur->data;
}


void VISIBLE_ATTR *ws_list_seek(list_t *l, const void *indicator){
    struct list_entry* cur = l->first;
    
    if ( cur == 0 ){
        return 0;
    }
     
    while( cur->next != 0 ){
        if ( l->seeker( cur->data, indicator ) == 1 ){
            return cur->data;
        }
        cur = cur->next;
    }    
    
    if ( l->seeker( cur->data, indicator ) == 1 ){
        return cur->data;
    }
    
    return 0;
}



unsigned int VISIBLE_ATTR ws_list_size(const list_t *l){ 
    return l->element_count; 
}

int VISIBLE_ATTR ws_list_empty(const list_t *l) {
    return (l->element_count == 0);
}

