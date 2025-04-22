/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/


#ifndef _WEBSERVER_SSL_H_
#define _WEBSERVER_SSL_H_



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
int					WebserverSSLRecvNonBlocking(socket_info* s, unsigned char *buf, unsigned len, int flags);
int					WebserverSSLCloseSockets(socket_info *s);

#else

	#define  WebserverSSLPending( a ) 0

#endif



#ifdef __cplusplus
}
#endif


#endif
