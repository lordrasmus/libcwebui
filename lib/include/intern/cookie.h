/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _COOKIE_H_
#define _COOKIE_H_

#ifdef __GNUC__
	#include "dataTypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int checkCookie(char *name,char *value,HttpRequestHeader *header);

void copyCookieValue(char *line,HttpRequestHeader *header,int pos,int pos2);

void parseCookies( char *line,int length,HttpRequestHeader *header);

#ifdef __cplusplus
}
#endif

#endif
