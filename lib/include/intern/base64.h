/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_BASE64_H_
#define _WEBSERVER_BASE64_H_


#ifdef __cplusplus
extern "C" {
#endif


void                WebserverBase64Encode(const unsigned char *input, int length,unsigned char *output,SIZE_TYPE out_length);


#ifdef __cplusplus
}
#endif


#endif
