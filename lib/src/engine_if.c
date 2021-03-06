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

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif



/*
#define IF_DEBUG
*/

static int find_if_tags_pos ( const char *pagedata,const int datalenght,int* else_tag_pos,int* endif_tag_pos ) {
    int pos_start=0,level=0,offset=0;
    TAG_IDS tag_id;

#ifdef IF_DEBUG
    char *bb=pagedata;
    char bb1[10000];
#endif

    *else_tag_pos = -1;
    *endif_tag_pos = -1;

    do {
#ifdef IF_DEBUG
        memcpy ( bb1,pagedata+offset,datalenght-offset );
        bb1[datalenght-offset]='\0';
        bb = &pagedata[offset];
#endif

        pos_start = getNextTag ( &pagedata[offset],datalenght-offset,IF_TAG_GROUP,&tag_id );
        if ( pos_start > -1 ) {
            switch ( tag_id ) {
            case IF_TAG :
                level++;
                break;

            case ELSE_TAG:
                if ( level == 0 ) {
                    *else_tag_pos = offset + pos_start;
                }
                break;

            case ENDIF_TAG:
                if ( level-- == 0 ) {
                    *endif_tag_pos = offset + pos_start;
                    return *endif_tag_pos;
                }
                break;

            default:
                LOG ( TEMPLATE_LOG,ERROR_LEVEL,0,"Unerwartete TAG ID %X",tag_id );
                return -1;
            }
        } else {
            return -1;
        }

        offset+=pos_start;
    } while ( level >= 0 );

    return -1;

}






void engine_TemplateIF ( http_request *s,const char* prefix ,const char *pagename,const char *pagedata,const int datalenght,FUNCTION_PARAS* func,int* i ) {
    int else_tag_pos,endif_tag_pos;
    CONDITION_RETURN cond_ret;
#ifdef IF_DEBUG
    char bb1[5000];
    char bb2[5000];
#endif


    int then_length=0;
    int else_length=0;

    if ( 0>find_if_tags_pos ( pagedata,datalenght,&else_tag_pos,&endif_tag_pos ) ) {
        LOG ( TEMPLATE_LOG,ERROR_LEVEL,s->socket->socket,"%s","if tags nicht gefunden" );
	return;
    }

    then_length = endif_tag_pos -7;

#ifdef IF_DEBUG
    memcpy ( bb1,pagedata,then_length );
    bb1[then_length]='\0';
#endif

    cond_ret = engine_callCondition ( s,func );


    if ( cond_ret == CONDITION_ERROR ) {
        *i+=endif_tag_pos;
        return;
    }

    if ( else_tag_pos != -1 ) {
        then_length = else_tag_pos -6;
        else_length = endif_tag_pos-else_tag_pos -7;

#ifdef IF_DEBUG
        memcpy ( bb1,pagedata,then_length );
        bb1[then_length]='\0';
        memcpy ( bb2,pagedata+else_tag_pos,else_length );
        bb2[else_length]='\0';
#endif

        switch ( cond_ret ) {
        case CONDITION_TRUE :
#ifdef _WEBSERVER_TEMPLATE_DEBUG_
            WebServerPrintf ( "Builtin TRUE\n" );
#endif
            s->engine_current->return_found = processHTML ( s,prefix,pagename,pagedata,then_length );
            break;

        case CONDITION_FALSE :
#ifdef _WEBSERVER_TEMPLATE_DEBUG_
            WebServerPrintf ( "Builtin FALSE\n" );
#endif
            s->engine_current->return_found = processHTML ( s,prefix,pagename,pagedata+else_tag_pos,else_length );
            break;

        default:
            LOG ( TEMPLATE_LOG,ERROR_LEVEL,0,"%s","Condition Error" );
            break;

        }

        *i+=endif_tag_pos;
        return;

    } else {
        switch ( cond_ret ) {
        case CONDITION_TRUE :
#ifdef _WEBSERVER_TEMPLATE_DEBUG_
            WebServerPrintf ( "Builtin TRUE\n" );
#endif
            s->engine_current->return_found = processHTML ( s,prefix,pagename,pagedata,then_length );
            break;

        case CONDITION_FALSE : /* kein else zweig vorhanden */
            break;
        default:
            LOG ( TEMPLATE_LOG,ERROR_LEVEL,0,"%s","Condition Error");
            break;
        }
    }


    *i+=endif_tag_pos;

}


