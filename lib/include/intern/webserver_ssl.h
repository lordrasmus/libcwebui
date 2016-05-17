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


#ifndef _WEBSERVER_SSL_H_
#define _WEBSERVER_SSL_H_


#include "webserver.h"



#define SSL_SHA_DIG_LEN 20

struct sha_context;




#ifdef __cplusplus
extern "C" {
#endif

#ifdef WEBSERVER_USE_SSL

int 				initOpenSSL(void);

int                 WebserverSSLTestKeyfile( char* keyfile );
int					WebserverSSLInit(socket_info* s);
int					WebserverSSLAccept(socket_info* s);

int 				WebserverSSLPending(socket_info* s);
SOCKET_SEND_STATUS	WebserverSSLSendNonBlocking(socket_info* s, const unsigned char *buf, int len, int flags,int* bytes_send);
int					WebserverSSLRecvNonBlocking(socket_info* s, unsigned char *buf, int len, int flags);
int					WebserverSSLCloseSockets(socket_info *s);




int                 WebserverSHA1(const unsigned char* data,size_t len,unsigned char* md);
struct sha_context* WebserverSHA1Init(void);
int                 WebserverSHA1Update(struct sha_context* ssl_context,const void* data,size_t len);
int                 WebserverSHA1Final(struct sha_context* sha_ctx,unsigned char* data);
void                WebserverSHA1Free(struct sha_context* sha_ctx );

int                 WebserverMD5(const unsigned char* data,size_t len,unsigned char* md);
int                 WebserverRANDBytes(unsigned char *buf, int num);
void                WebserverBase64Encode(const unsigned char *input, int length,unsigned char *output,SIZE_TYPE out_length);

#endif

#ifdef __cplusplus
}
#endif


#endif
