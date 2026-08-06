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

/* Number of decrypted bytes waiting in the SSL layer, 0 if none.
 *
 * The count matters, not just "something is there": run_extern_handler() tells
 * a handler that made progress from one that did nothing by comparing the
 * value across a call. Backends must return the real number - openssl and
 * mbedtls used to collapse it to 0/1, which made that comparison blind to a
 * handler consuming only part of what was pending. Also updates
 * socket_info::ssl_pending_bytes. */
int 				WebserverSSLPending(socket_info* s);
SOCKET_SEND_STATUS	WebserverSSLSendNonBlocking(socket_info* s, const unsigned char *buf, size_t len, int flags, size_t* bytes_send);
int					WebserverSSLRecvNonBlocking(socket_info* s, unsigned char *buf, unsigned len, int flags);
int					WebserverSSLCloseSockets(socket_info *s);

#else

	#define  WebserverSSLPending( a ) 0

#endif



#ifdef __cplusplus
}
#endif


#endif
