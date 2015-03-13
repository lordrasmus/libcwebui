/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#include "WebserverConfig.h"

#ifdef WEBSERVER_USE_YASSL_CRYPTO


#ifdef __GNUC__

#include "system.h"
#include "webserver_log.h"
#include "header.h"
#include "globals.h"
#endif

#include "webserver_ssl.h"

// für das yassl base64encode
#define HAVE_WEBSERVER 

#define HAVE_CONFIG_H

#include "openssl_compat/ssl.h"
#include "ctc_md5.h"
#include "ctc_sha.h"
#include "ctc_coding.h"



#if SHA_DIGEST_LENGTH != SSL_SHA_DIG_LEN
	#warning "check yaSSL SHA Digest Length"
#endif

struct ssl_store_s {
	SSL *ssl;
};

struct sha_context {
	Sha *sha_ctx;
};

SSL_CTX *ctx;


#define KEYFILE "server.pem"
#define PASSWORD "password"
#define CA_LIST "root.pem"
#define DHFILE "dh1024.pem"

//BIO *bio_err = 0;
static char *pass;
//static int password_cb(char *buf,int num, int rwflag,void *userdata);
//static void sigpipe_handle(int x);

SSL_CTX *initialize_ctx(char* path, char *keyfile, char* ca, char *password);
int load_dh_params(SSL_CTX *ctx, char *file);
void printSSLErrorQueue(socket_info* s);

int initOpenSSL(void) {
	long cache_mode;
	char buffer[200];

	OpenSSL_add_all_algorithms(); // load & register cryptos
	SSL_load_error_strings(); // load all error messages
	ctx = initialize_ctx(globals.config.ssl_file_path, KEYFILE, CA_LIST, (char*) PASSWORD);
	snprintf(buffer, 200, "%s%s", globals.config.ssl_file_path, DHFILE);
	if (load_dh_params(ctx, buffer) < 0) {
		LOG( CONNECTION_LOG, ERROR_LEVEL, 0, "SSL fehler dh_params", "");
		return -1;
	}

	cache_mode = SSL_SESS_CACHE_OFF; // No session caching for client or server takes place.
	//cache_mode = SSL_SESS_CACHE_SERVER|SSL_SESS_CACHE_NO_INTERNAL;
	SSL_CTX_set_session_cache_mode( ctx, cache_mode);

	//SSL_CTX_sess_set_new_cb(ctx,    ssl_callback_NewSessionCacheEntry);
	//SSL_CTX_sess_set_get_cb(ctx,    ssl_callback_GetSessionCacheEntry);
	//SSL_CTX_sess_set_remove_cb(ctx, ssl_callback_DelSessionCacheEntry);

	SSL_CTX_set_mode( ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	//SSL_CTX_set_read_ahead( ctx, 1);
	#warning "yaSSL kennt SSL_CTX_set_read_ahead nicht mögliche sende buffer offset probleme"

	SSL_CTX_set_options( ctx, SSL_OP_NO_SSLv2);
	// SSLv2 deaktivieren

	return 0;
}

static int password_cb(char *buf, int num, int rwflag, void *userdata) {
	if (num < (int) strlen(pass) + 1) return (0);

	strncpy(buf, pass, num);
	return (strlen(pass));
}

int load_dh_params(SSL_CTX *ctx, char *file) {
	/*DH *ret = 0;
	BIO *bio;

	if ((bio = BIO_new_file(file, "r")) == NULL) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Couldn't open DH file", "");
		return -1;
	}

	ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
	BIO_free(bio);
	if (SSL_CTX_set_tmp_dh ( ctx,ret ) < 0) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Couldn't set DH parameters", "");
		return -1;
	}*/
	#warning "load_dh_params mit yaSSL implemetieren"

	return 1;
}

SSL_CTX *initialize_ctx(char* path, char *keyfile, char* ca, char *password) {
	SSL_METHOD *meth;
	SSL_CTX *ctx;
	char buffer[200];

#ifdef jojo
	if (!bio_err) {
		/* Global system initialization*/
		SSL_library_init();
		SSL_load_error_strings();

		/* An error write context */
		bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
	}

	/* Set up a SIGPIPE handler */
	//signal(SIGPIPE,sigpipe_handle);
	/* Create our context*/

#endif
	#warning "yaSSL noch initialiseren"
	meth = (SSL_METHOD*) SSLv23_server_method();
	ctx = SSL_CTX_new(meth);

	/* Load our keys and certificates*/
	snprintf(buffer, 200, "%s%s", path, keyfile);
	if (!(SSL_CTX_use_certificate_chain_file(ctx, buffer) )) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read certificate file %s", buffer);
		return NULL;
	}

	pass = password;
	SSL_CTX_set_default_passwd_cb(ctx, password_cb);
	if (!(SSL_CTX_use_PrivateKey_file(ctx, buffer, SSL_FILETYPE_PEM) )) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read key file %s", buffer);
		return NULL;
	}

	snprintf(buffer, 200, "%s%s", path, ca);
	if (!(SSL_CTX_load_verify_locations(ctx, buffer, 0) )) // Load the CAs we trust
			{
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read CA list %s", buffer);
		return NULL;
	}

	// TLS RSA AES_256_CBC SHA
//	#define CIPHER_LIST	"ALL:!aNULL:!eNULL:-HIGH"

#define  CIPHER_LIST	"AES256-SHA:AES128-SHA:DHE-DSS-AES128-SHA:DHE-DSS-AES256-SHA:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA:ADH-AES128-SHA:ADH-AES256-SHA" //	 DHE_RSA:NULL-MD5:NULL-SHA"
	//#define CIPHER_LIST	"TLS_RSA_WITH_AES_128_CBC_SHA"
	//#define CIPHER_LIST		"SSL_TXT_DES_192_EDE3_CBC_WITH_SHA:-HIGH"
	//#define CIPHER_LIST		"LOW:TLS_RSA_WITH_AES_128_CBC_SHA"

	if (!SSL_CTX_set_cipher_list(ctx, CIPHER_LIST)) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't set cipher list %s", CIPHER_LIST);
		return NULL;
	}

	//SSL_CTX_set_session_id_context(ctx, &sid_ctx, sizeof(sid_ctx));

	return ctx;
}

void destroy_ctx(SSL_CTX *ctx) {
	SSL_CTX_free(ctx);
}

int WebserverSSLInit(socket_info* s) {
	if (s->ssl_context == 0) s->ssl_context = (struct ssl_store_s*) WebserverMalloc ( sizeof ( struct ssl_store_s ),0 );
	s->ssl_context->ssl = SSL_new(ctx);
	if (s->ssl_context->ssl == 0)
	LOG(CONNECTION_LOG, ERROR_LEVEL, s->socket, "WebserverSSLInit fehler", "");
	if (SSL_set_fd(s->ssl_context->ssl, s->socket) != 1) return 111;
	return 0;
}

int WebserverSSLCloseSockets(socket_info *s) {
	ERR_clear_error();
	if (s->ssl_context != 0) {
		SSL_set_shutdown(s->ssl_context->ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
		SSL_free(s->ssl_context->ssl);
		WebserverFree(s->ssl_context);
	}
	return 0;
}

void printSSLErrorQueue(socket_info* s) {
	unsigned long err_code;
	char buffer[130]; // SSL requires 120 bytes
	while ((err_code = ERR_get_error())) {
		ERR_error_string(err_code, buffer);
		LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "%s", buffer);
	}
}

int WebserverSSLAccept(socket_info* s) {
	int r, r2;

	while (1) {
		ERR_clear_error();
		r = SSL_accept(s->ssl_context->ssl);
		if (r == 1) {
#if _WEBSERVER_CONNECTION_DEBUG_ >= 3
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,s->socket,"SSL accept ok","" );
#endif
			s->run_ssl_accept = 0;
			printSSLErrorQueue(s);
			return SSL_ACCEPT_OK;
		}

		if (r <= 0) {
			r2 = SSL_get_error(s->ssl_context->ssl, r);
			switch (r2) {
			case SSL_ERROR_NONE:
				// The TLS/SSL I/O operation completed. This result code is returned if and only if ret > 0.
				break;

			case SSL_ERROR_WANT_READ:
				printSSLErrorQueue(s);
#if _WEBSERVER_CONNECTION_DEBUG_ >= 5
				LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_WANT_READ","" );
#endif
				return CLIENT_NO_MORE_DATA;
				break;
			case SSL_ERROR_WANT_WRITE:
				// The operation did not complete; the same TLS/SSL I/O function should be called again later.
				// If, by then, the underlying BIO has data available for reading (if the result code is SSL_ERROR_WANT_READ)
				// or allows writing data (SSL_ERROR_WANT_WRITE), then some TLS/SSL protocol progress will take place,
				// i.e. at least part of an TLS/SSL record will be read or written.
				// Note that the retry may again lead to a SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE condition.
				// There is no fixed upper limit for the number of iterations that may be necessary until progress becomes visible at application protocol level.

				// For socket BIOs (e.g. when SSL_set_fd() was used), select() or poll() on the underlying socket can be used to find out
				// when the TLS/SSL I/O function should be retried.
				// Caveat: Any TLS/SSL I/O function can lead to either of SSL_ERROR_WANT_READ and SSL_ERROR_WANT_WRITE. In particular,
				// SSL_read() or SSL_peek() may want to write data and SSL_write() may want to read data.
				// This is mainly because TLS/SSL handshakes may occur at any time during the protocol (initiated by either the client or the server);
				// SSL_read(), SSL_peek(), and SSL_write() will handle any pending handshakes.
				printSSLErrorQueue(s);
#if _WEBSERVER_CONNECTION_DEBUG_ >= 4
				LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_WANT_WRITE","" );
#endif
				return CLIENT_NO_MORE_DATA;
				break;

			case SSL_ERROR_WANT_CONNECT:
			case SSL_ERROR_WANT_ACCEPT:
				// The operation did not complete; the same TLS/SSL I/O function should be called again later.
				// The underlying BIO was not connected yet to the peer and the call would block in connect()/accept().
				// The SSL function should be called again when the connection is established.
				// These messages can only appear with a BIO_s_connect() or BIO_s_accept() BIO, respectively.
				// In order to find out, when the connection has been successfully established, on many platforms select() or poll()
				// for writing on the socket file descriptor can be used.
			case SSL_ERROR_ZERO_RETURN:
				// The TLS/SSL connection has been closed. If the protocol version is SSL 3.0 or TLS 1.0,
				// this result code is returned only if a closure alert has occurred in the protocol,
				// i.e. if the connection has been closed cleanly.
				// Note that in this case SSL_ERROR_ZERO_RETURN does not necessarily indicate that the underlying transport has been closed.
			case SSL_ERROR_WANT_X509_LOOKUP:
				// The operation did not complete because an application callback set by SSL_CTX_set_client_cert_cb() has asked to be called again.
				// The TLS/SSL I/O function should be called again later. Details depend on the application.
			case SSL_ERROR_SYSCALL:
				// Some I/O error occurred. The OpenSSL error queue may contain more information on the error.
				// If the error queue is empty (i.e. ERR_get_error() returns 0), ret can be used to find out more about the error:
				// If ret == 0, an EOF was observed that violates the protocol. If ret == -1, the underlying BIO reported an I/O error
				// (for socket I/O on Unix systems, consult errno for details).
				LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "SSL_ERROR_SYSCALL", "");
				printSSLErrorQueue(s);
				return SSL_PROTOCOL_ERROR;
			case SSL_ERROR_SSL:
				LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "SSL_ERROR_SSL", "");
				printSSLErrorQueue(s);
				return SSL_PROTOCOL_ERROR;
				break;
				// A failure in the SSL library occurred, usually a protocol error. The OpenSSL error queue contains more information on the error.
			default:
				printSSLErrorQueue(s);
				LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "Unhandled SSL Error ( %d )", r2);
				return SSL_PROTOCOL_ERROR;
				break;
			}
		}
	}

	s->use_ssl = 0;
	return NO_SSL_CONNECTION_ERROR;
}

int WebserverSLLRecvNonBlocking(socket_info* s, unsigned char *buf, int len, int flags) {
	int ret = 0;
	int l = 0;
	int r2;
	unsigned long err_code;
	//unsigned long ssl_err;
	char buffer[130]; // SSL requires 120 bytes
	do {
		ERR_clear_error();
		ret = SSL_read(s->ssl_context->ssl, &buf[l], len);
		r2 = SSL_get_error(s->ssl_context->ssl, ret);
		switch (r2) {
		case SSL_ERROR_NONE:
			l += ret;
			break;

		case SSL_ERROR_WANT_READ:
			if (l == 0) {
				//LOG(CONNECTION_LOG,ERROR_LEVEL,"(%d) WebserverSLLRecvNonBlocking : SSL_ERROR_WANT_READ",s->socket);
				return CLIENT_NO_MORE_DATA;
			} else {
				return l;
			}
			break;
		case SSL_ERROR_WANT_WRITE:
			err_code = ERR_get_error();
			ERR_error_string(err_code, buffer);
			LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "SSL_ERROR_WANT_WRITE", "");
			if (l == 0) {
				return CLIENT_NO_MORE_DATA;
			} else {
				return l;
			}
			break;

		case SSL_ERROR_ZERO_RETURN:
			err_code = ERR_get_error();
			ERR_error_string(err_code, buffer);
#ifdef _WEBSERVER_CONNECTION_DEBUG_
			LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_ZERO_RETURN","" );
#endif
			return CLIENT_DISCONNECTED;

		case SSL_ERROR_SYSCALL:
			err_code = ERR_get_error();
			ERR_error_string(err_code, buffer);
#ifdef _WEBSERVER_CONNECTION_DEBUG_
			LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_SYSCALL ( %d ) %s",r2,buffer );
#endif
			return SSL_PROTOCOL_ERROR;

		case SSL_ERROR_WANT_X509_LOOKUP:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
		case SSL_ERROR_SSL:
			// A failure in the SSL library occurred, usually a protocol error. The OpenSSL error queue contains more information on the error.
		default:
			err_code = ERR_get_error();
			ERR_error_string(err_code, buffer);
			//ERR_error_string(r2,buffer);
#ifdef _WEBSERVER_CONNECTION_DEBUG_
			LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"Unhandled SSL Error ( %d ) %s",r2,buffer );
#endif
			return SSL_PROTOCOL_ERROR;
		}
		if (ret == len) {
			return ret;
		}
		//}while(SSL_pending(s->ssl)>0);
	} while (1);

	return SSL_PROTOCOL_ERROR;
}

SOCKET_SEND_STATUS WebserverSLLSendNonBlocking(socket_info* s, unsigned char *buf, int len, int flags, int* bytes_send) {
	int ret = 0;
	int l = 0;
	int r2;
	unsigned long err_code;
	char buffer[130]; // SSL requires 120 bytes
	do {
		ERR_clear_error();
		ret = SSL_write(s->ssl_context->ssl, &buf[l], len - l);
		r2 = SSL_get_error(s->ssl_context->ssl, ret);
		switch (r2) {
		case SSL_ERROR_NONE:
			l += ret;
			break;

		case SSL_ERROR_WANT_READ:
			LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "SSL_ERROR_WANT_READ", "");
			if (bytes_send != 0) *bytes_send = l;
			return SOCKET_SEND_SEND_BUFFER_FULL;

		case SSL_ERROR_WANT_WRITE:
			//LOG(CONNECTION_LOG,ERROR_LEVEL,"(%d) WebserverSLLSendNonBlocking : SSL_ERROR_WANT_WRITE",s->socket);
			if (bytes_send != 0) *bytes_send = l;
			return SOCKET_SEND_SEND_BUFFER_FULL;

		case SSL_ERROR_SSL:
		case SSL_ERROR_WANT_X509_LOOKUP:
		case SSL_ERROR_SYSCALL:
		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
		default:
			err_code = ERR_get_error();
			ERR_error_string(err_code, buffer);
			//ERR_error_string(r2,buffer);
			LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "Unhandled SSL Error ( %d ) %s", r2, buffer);
			if (bytes_send != 0) *bytes_send = l;
			return SOCKET_SEND_SSL_ERROR;
		}
		if (l == len) {
			if (bytes_send != 0) *bytes_send = l;
			return SOCKET_SEND_NO_MORE_DATA;
		}
		//}while(SSL_pending(s->ssl)>0);
	} while (1);
	return SOCKET_SEND_SSL_ERROR;
}

unsigned char* WebserverSHA1(const unsigned char* data, size_t len, unsigned char* md) {
	Sha sha;
	InitSha(&sha);
	ShaUpdate(&sha, buffer, sizeof(buffer)); // can be called again and	
	ShaFinal(&sha, shaSum);
}

struct sha_context* WebserverSHA1Init() {
	struct sha_context* sctx;	
	sctx = (struct sha_context*) WebserverMalloc ( sizeof ( struct sha_context ),0 );
	sctx->sha_ctx = (Sha*) WebserverMalloc ( sizeof ( Sha ),0 );
	InitSha(sctx->sha_ctx);	
	return sctx;
}

int WebserverSHA1Update(struct sha_context* sha_ctx, const void* data, size_t len) {
	ShaUpdate(sha_ctx->sha_ctx, (const byte*)data, len);
	return 0;
}

int WebserverSHA1Final(struct sha_context* sha_ctx, unsigned char* data) {
	ShaFinal(sha_ctx->sha_ctx,data);
	return 0;
}

unsigned char* WebserverMD5(const unsigned char* data, size_t len, unsigned char* md) {
	Md5 md5;
	InitMd5(&md5);
	Md5Update(&md5, data, len); // can be called again and
	Md5Final(&md5, md);
	return 0;
}

int WebserverRANDBytes(unsigned char *buf, int num) {
	return RAND_bytes(buf, num);
}

void WebserverBase64(const unsigned char *input, int length, unsigned char *output, int out_length) {
	word32 outLen;
	Base64Encode(input, length, output,  &outLen);
	if ( outLen > out_length)
		printf("-------- WARNING ----------- Ausgabe Buffer für Base64 zu kein\n");

}

#endif // WEBSERVER_USE_SSL

