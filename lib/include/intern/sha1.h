/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifndef WEBSERVER_SHA1_H
#define WEBSERVER_SHA1_H

#define SSL_SHA_DIG_LEN 20

struct sha_context;


#ifdef __cplusplus
extern "C" {
#endif

int                 WebserverSHA1(const unsigned char* data,size_t len,unsigned char* md);
struct sha_context* WebserverSHA1Init(void);
int                 WebserverSHA1Update(struct sha_context* sha_ctx,const void* data,size_t len);
int                 WebserverSHA1Final(struct sha_context* sha_ctx,unsigned char* data);
void                WebserverSHA1Free(struct sha_context* sha_ctx );

#ifdef __cplusplus
}
#endif

#endif
