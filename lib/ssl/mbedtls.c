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

#ifdef WEBSERVER_USE_MBEDTLS_CRYPTO

#include "webserver.h"
#include "mbedtls/version.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/platform.h"
#include "mbedtls/timing.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif


/*#if SHA_DIGEST_LENGTH != SSL_SHA_DIG_LEN
	#error "SHA Digest Length mismatch"
#endif*/

#if (MBEDTLS_VERSION_NUMBER < 0x02040200)
	#error "Must use mbed TLS 2.4.2 or later"
#endif



struct ssl_store_s {
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_ssl_cookie_ctx cookie_ctx;
	mbedtls_x509_crt srvcert;
	mbedtls_pk_context pkey;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_timing_delay_context timer;
	mbedtls_net_context client_fd;
	
	//BIO *sbio;
	unsigned int read_pending;
};


const char *pers = "dtls_server";


static void my_debug( void *ctx, int level, const char *file, int line, const char *str ){
    ((void) level);
	printf("my_debug :%s:%04d: %s", file, line, str );
    //mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    //fflush(  (FILE *) ctx  );
}

int initOpenSSL(void) {	
	LOG( SSL_LOG, NOTICE_LEVEL, 0, "using mbed tls : %s ( support is still experimental \n",MBEDTLS_VERSION_STRING);
	return 0;
}

static void init_ctx( struct ssl_store_s *ctx){

	mbedtls_ssl_init( &ctx->ssl );
	
    mbedtls_ssl_config_init( &ctx->conf );
    mbedtls_ssl_cookie_init( &ctx->cookie_ctx );
    
    mbedtls_x509_crt_init( &ctx->srvcert );
    mbedtls_pk_init( &ctx->pkey );
    mbedtls_entropy_init( &ctx->entropy );
    mbedtls_ctr_drbg_init( &ctx->ctr_drbg );
    
    mbedtls_debug_set_threshold( 2 );
    
    int ret = mbedtls_x509_crt_parse( &ctx->srvcert, (const unsigned char *) mbedtls_test_srv_crt, mbedtls_test_srv_crt_len );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

    ret = mbedtls_x509_crt_parse( &ctx->srvcert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

    ret =  mbedtls_pk_parse_key( &ctx->pkey, (const unsigned char *) mbedtls_test_srv_key, mbedtls_test_srv_key_len, NULL, 0 );
    if( ret != 0 )
    {
        printf( " failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret );
        goto exit;
    }
    
    if( ( ret = mbedtls_ctr_drbg_seed( &ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, (const unsigned char *) pers, strlen( pers ) ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }
    
    if( ( ret = mbedtls_ssl_config_defaults( &ctx->conf, MBEDTLS_SSL_IS_SERVER,  MBEDTLS_SSL_TRANSPORT_STREAM,   MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_conf_rng( &ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg );
    mbedtls_ssl_conf_dbg( &ctx->conf, my_debug, stdout );
    
    mbedtls_ssl_conf_ca_chain( &ctx->conf, ctx->srvcert.next, NULL );
    if( ( ret = mbedtls_ssl_conf_own_cert( &ctx->conf, &ctx->srvcert, &ctx->pkey ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_cookie_setup( &ctx->cookie_ctx,
                                  mbedtls_ctr_drbg_random, &ctx->ctr_drbg ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_cookie_setup returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_conf_dtls_cookies( &ctx->conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check,
                               &ctx->cookie_ctx );

    if( ( ret = mbedtls_ssl_setup( &ctx->ssl, &ctx->conf ) ) != 0 )
    {
        printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_set_timer_cb( &ctx->ssl, &ctx->timer, mbedtls_timing_set_delay,
                                            mbedtls_timing_get_delay );
    

	return ;
	
exit:
	exit(1);
#if 0
	cache_mode = SSL_SESS_CACHE_OFF; // No session caching for client or server takes place.
	//cache_mode = SSL_SESS_CACHE_SERVER|SSL_SESS_CACHE_NO_INTERNAL;
	SSL_CTX_set_session_cache_mode(g_ctx, cache_mode);	
	SSL_CTX_set_mode(g_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	SSL_CTX_set_read_ahead(g_ctx, 1);
	// SSLv2, SSLv3 deaktivieren , testen :   nmap --script ssl-cert,ssl-enum-ciphers -p 443 192.168.11.94
	SSL_CTX_set_options(g_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

	#endif

	return ;
}


int VISIBLE_ATTR WebserverSSLTestKeyfile( char* keyfile ){
	#if 0
	SSL_METHOD *meth;
	SSL_CTX *ctx;

	int error = 0;
	meth = (SSL_METHOD*) SSLv23_server_method();
	ctx = SSL_CTX_new(meth);

	if (!(SSL_CTX_use_certificate_chain_file(ctx, keyfile))) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read certificate: file %s", keyfile);
		error = 1;
	}

	if (!(SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM))) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read key: file %s", keyfile);
		error = 1;
	}

	#warning Memory Leak

	return error;
	#endif
}
#if 0

SSL_CTX *initialize_ctx( char *keyfile, char* keyfile_backup, char* ca, char *password) {
	SSL_METHOD *meth;
	SSL_CTX *ctx;
	//char buffer[200];

	if (!bio_err) {
		SSL_library_init();
		SSL_load_error_strings();

		/* An error write context */
		bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
	}

	/* Create our context*/
	meth = (SSL_METHOD*) SSLv23_server_method();
	ctx = SSL_CTX_new(meth);

	/* Load our keys and certificates*/
	pass = password; // global char*

	if ( 0 == WebserverSSLTestKeyfile( keyfile ) ) {
		SSL_CTX_use_certificate_chain_file(ctx, keyfile);
		SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM);

	}else{
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read keyfile file %s trying backup", keyfile);

		if ( 0 == WebserverSSLTestKeyfile( keyfile_backup ) ) {
			SSL_CTX_use_certificate_chain_file(ctx, keyfile_backup);
			SSL_CTX_use_PrivateKey_file(ctx, keyfile_backup, SSL_FILETYPE_PEM);
		}else{
			LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read keyfile backup file %s ", keyfile_backup);
			return NULL;
		}
	}

	if (!(SSL_CTX_load_verify_locations(ctx, ca, 0))) // Load the CAs we trust
	{
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't read CA list %s", ca);
		return NULL;
	}

	// TLS RSA AES_256_CBC SHA
//	#define CIPHER_LIST	"ALL:!aNULL:!eNULL:-HIGH"

#define  CIPHER_LIST	"HIGH:!aNULL:!eNULL:!3DES:@STRENGTH"
//#define  CIPHER_LIST	"HIGH:!SSLv2:!SSLv3:RC4+HIGH:!aNULL:!eNULL:!3DES:@STRENGTH"
/*#define  CIPHER_LIST	"AES256-SHA:AES128-SHA:"\
						"DHE-DSS-AES128-SHA:DHE-DSS-AES256-SHA:"\
						"DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA"
	*/
	// ADH-AES128-SHA <- broken laut nmap
	// ADH-AES256-SHA <- broken laut nmap
	// DHE_RSA:NULL-MD5:NULL-SHA"

	// TLS_DH_anon_WITH_AES_128_CBC_SHA
	//#define CIPHER_LIST	"TLS_RSA_WITH_AES_128_CBC_SHA"
	//#define CIPHER_LIST		"SSL_TXT_DES_192_EDE3_CBC_WITH_SHA:-HIGH"
	//#define CIPHER_LIST		"LOW:TLS_RSA_WITH_AES_128_CBC_SHA"

	if (!SSL_CTX_set_cipher_list(ctx, CIPHER_LIST)) {
		LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "Can't set cipher list %s", CIPHER_LIST);
		return NULL;
	}

	//SSL_CTX_set_session_id_context(ctx, &sid_ctx, sizeof(sid_ctx));

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
	SSL_CTX_set_verify_depth(ctx, 1);
#endif

	return ctx;
}



void destroy_ctx(SSL_CTX *ctx) {
	SSL_CTX_free(ctx);
	
}

#endif

int WebserverSSLInit(socket_info* s) {
	
	printf("WebserverSSLInit\n");
	
	s->ssl_context = (struct ssl_store_s*) WebserverMalloc ( sizeof ( struct ssl_store_s ) );
	
	init_ctx( s->ssl_context );
	mbedtls_net_init( &s->ssl_context->client_fd );
	s->ssl_context->client_fd.fd = s->socket;
	s->ssl_context->read_pending = 0;
	
    //mbedtls_ssl_set_bio( &s->ssl_context->ssl, &s->ssl_context->client_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout );
    mbedtls_ssl_set_bio( &s->ssl_context->ssl, &s->ssl_context->client_fd, mbedtls_net_send, mbedtls_net_recv, 0 );

	#if 0
	SSL_set_read_ahead(s->ssl_context->ssl, 0);
	#endif
	return 0;
}

int WebserverSSLCloseSockets(socket_info *s) {
	if (s->ssl_context != 0) {
		//SSL_set_shutdown(s->ssl_context->ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
		//SSL_free(s->ssl_context->ssl);
		WebserverFree(s->ssl_context);
	}
	return 0;
}

void printSSLErrorQueue(socket_info* s) {
	#if 0
	unsigned long err_code;
	char buffer[130]; // SSL requires 120 bytes
	while ((err_code = ERR_get_error())) {
		ERR_error_string(err_code, buffer);
		LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "%s", buffer);
	}
	#endif
}

int WebserverSSLPending(socket_info* s) {
	
	s->ssl_context->read_pending = s->ssl_context->ssl.in_msglen;

	printf("WebserverSSLPending : %d\n",s->ssl_context->read_pending);


	if( s->ssl_context->read_pending > 0 ){
		return 1;
	}

	return 0;
}

char debug_buffer[1000];

static char* print_mbedtls_status( int ret ){
	
	if ( ret > 0 ){
		sprintf( debug_buffer, "mbed tls status : ( %d ) MBEDTLS_OK ",ret);
		return debug_buffer;
	}
	
	sprintf( debug_buffer, "mbed tls status : ( 0x%X ) ",ret*-1);
	
	switch ( ret ){
		case MBEDTLS_ERR_SSL_BAD_INPUT_DATA:
			strcat( debug_buffer, "MBEDTLS_ERR_SSL_BAD_INPUT_DATA");
			break;
		case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO:
			strcat( debug_buffer, "MBEDTLS_ERR_SSL_BAD_HS_CLIENT_HELLO");
			break;
		case MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE:
			strcat( debug_buffer, "MBEDTLS_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE");
			break;
			
		case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
			strcat( debug_buffer, "MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY");
			break;
		
		case MBEDTLS_ERR_SSL_WANT_READ:
			strcat( debug_buffer, "MBEDTLS_ERR_SSL_WANT_READ");
			break;
			
		case 0:
			strcat( debug_buffer, "MBEDTLS_OK");
			break;
		default:
			strcat( debug_buffer, "unknown");
			break;
	}
	
	return debug_buffer;
}

int WebserverSSLAccept(socket_info* s) {

    
    int ret;
    do {
		ret = mbedtls_ssl_handshake( &s->ssl_context->ssl );
	}while( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE );

	printf("WebserverSSLAccept : %s\n", print_mbedtls_status( ret ) );
    s->ssl_context->read_pending = 0;
    return SSL_ACCEPT_OK;
	
	#if 0
	while (1) {
		ERR_clear_error();
		r = SSL_accept(s->ssl_context->ssl);
		if (r == 1) {
#if _WEBSERVER_CONNECTION_DEBUG_ > 1
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,s->socket,"SSL accept ok","" );
#endif
			//PlatformSetNonBlocking(s->socket);
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
				//printSSLErrorQueue(s);

				//
				// Hier muss nur nochmal gelesen werden
				//
#if _WEBSERVER_CONNECTION_DEBUG_ > 4
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
//#if _WEBSERVER_CONNECTION_DEBUG_ >= 4
				LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_WANT_WRITE","" );
//#endif
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
	#endif
	return NO_SSL_CONNECTION_ERROR;
}

int WebserverSSLRecvNonBlocking(socket_info* s, unsigned char *buf, int len, UNUSED_PARA int flags) {
	
	int ret;
	
	//len = 20;
	
	if ( s->ssl_context->read_pending > 0 ){
		
		if ( s->ssl_context->read_pending <= len ){
			ret = mbedtls_ssl_read( &s->ssl_context->ssl, buf, s->ssl_context->read_pending );
		}else{
			ret = mbedtls_ssl_read( &s->ssl_context->ssl, buf, len );
		}
		if ( ret < 0 ){
			switch( ret ){
				case MBEDTLS_ERR_SSL_WANT_READ:
				case MBEDTLS_ERR_SSL_WANT_WRITE:
					return CLIENT_NO_MORE_DATA;
				
				default:
					printf( " mbedtls_ssl_read pending returned -0x%x\n\n", -ret );
					exit(1);
			}
		}
		printf("WebserverSSLRecvNonBlocking pending ( %d ) : %s\n",len,print_mbedtls_status(ret));
		buf[ret] = '\0';
		printf( " %d bytes read\n\n%s\n\n", ret, buf );
		return ret;
	}
	
	ret = mbedtls_ssl_read( &s->ssl_context->ssl, buf, len );
	

    if( ret <= 0 )
    {
		printf("WebserverSSLRecvNonBlocking ( %d ) : %s\n",len,print_mbedtls_status(ret));
        switch( ret )
        {
			case 0:
				return CLIENT_DISCONNECTED;
				
			case MBEDTLS_ERR_SSL_WANT_READ:
			case MBEDTLS_ERR_SSL_WANT_WRITE:
				return CLIENT_NO_MORE_DATA;
			
            case MBEDTLS_ERR_SSL_TIMEOUT:
                printf( " timeout\n\n" );
                return CLIENT_DISCONNECTED;

            case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                ret = 0;
                return CLIENT_DISCONNECTED;

            default:
                printf( " mbedtls_ssl_read returned -0x%x\n\n", -ret );
                exit(1);
        }
    }

out_read:
    len = ret;
    buf[len] = '\0';
    printf( " %d bytes read\n\n%s\n\n", len, buf );
    
    return len;
	
	#if 0
	int ret = 0;
	int l = 0;
	int r2;
	int diff;
	unsigned long err_code;

	if ( s->ssl_context->read_pending ){

		if ( ! WebserverSSLPending( s ) ){
			return 0;
		}

		if ( s->ssl_context->read_pending <= len ){
			ret = SSL_read(s->ssl_context->ssl, buf, s->ssl_context->read_pending );
			return ret;
		}else{
			ret = SSL_read(s->ssl_context->ssl, buf, len );
			return ret;
		}

	}

	char buffer[130]; // SSL requires 120 bytes
	do {
		ERR_clear_error();
		diff = len - l;
		ret = SSL_read(s->ssl_context->ssl, &buf[l], diff);
		//s->ssl_context->sbio
		r2 = SSL_get_error(s->ssl_context->ssl, ret);
		switch (r2) {
		case SSL_ERROR_NONE:
			l += ret;
			break;

		case SSL_ERROR_WANT_READ:

#if _WEBSERVER_CONNECTION_DEBUG_ > 3
			LOG(CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_WANT_READ %d / %d / %d / %d ",l,len,ret,r2);
#endif
			if (l == 0) {
				return CLIENT_NO_MORE_DATA;
			} else {
				return l;
			}
			break;
		case SSL_ERROR_WANT_WRITE:

#if _WEBSERVER_CONNECTION_DEBUG_ > 3
			LOG(CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_WANT_WRITE %d / %d / %d / %d ",l,len,ret,r2);
#endif

			if (l == 0) {
				return CLIENT_NO_MORE_DATA;
			} else {
				return l;
			}
			break;

		case SSL_ERROR_ZERO_RETURN:
			err_code = ERR_get_error();
			ERR_error_string(err_code, buffer);
//#ifdef _WEBSERVER_CONNECTION_DEBUG_
			LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_ZERO_RETURN","" );
//#endif
			return CLIENT_DISCONNECTED;

		case SSL_ERROR_SYSCALL:
			err_code = ERR_get_error();
			if ( err_code == 0 ){
				if ( ( ret == 0 ) && ( errno == 0 )){
					return CLIENT_DISCONNECTED;
				}
				#if _WEBSERVER_CONNECTION_DEBUG_ > 1
					LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_SYSCALL ( %d / %d / %d / %m ) ",ret,r2, errno );
				#endif
			}else{
				ERR_error_string(err_code, buffer);
				#if _WEBSERVER_CONNECTION_DEBUG_ > 1
					LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_SYSCALL ( %d / %d ) %s",ret,r2,buffer );
				#endif
			}
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
//#ifdef _WEBSERVER_CONNECTION_DEBUG_
			LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"Unhandled SSL Error ( %d ) %s",r2,buffer );
//#endif
			return SSL_PROTOCOL_ERROR;
		}
		if (l == len) {
			return l;
		}
		//}while(SSL_pending(s->ssl)>0);
	} while (1);
	#endif
	return SSL_PROTOCOL_ERROR;
}

SOCKET_SEND_STATUS WebserverSSLSendNonBlocking(socket_info* s, const unsigned char *buf, int len, UNUSED_PARA int flags, int* bytes_send) {
	int ret;
	
	
	ret = mbedtls_ssl_write( &s->ssl_context->ssl, buf, len );
	printf("WebserverSSLSendNonBlocking ( %d ): %s\n",len,print_mbedtls_status(ret));
	 
	if ( ret == len ){
		*bytes_send = len;
		return SOCKET_SEND_NO_MORE_DATA;
	}
	
	if ( ( ret > 0 ) && ( ret < len )){
		*bytes_send = ret;
		return SOCKET_SEND_SEND_BUFFER_FULL;
	}
	 
    if ( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ){
		*bytes_send = 0;
		return SOCKET_SEND_SEND_BUFFER_FULL;
	}
    
    *bytes_send = len;
    return SOCKET_SEND_NO_MORE_DATA;
           
	#if 0
	int ret = 0;
	int l = 0;
	int r2;
	unsigned long err_code;
	//unsigned short want_reads = 0;
	//unsigned short want_writes = 0;
	
	

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

#if _WEBSERVER_CONNECTION_DEBUG_ > 3
			LOG( CONNECTION_LOG, ERROR_LEVEL, s->socket, "SSL_ERROR_WANT_READ", "");
#endif
			if (bytes_send != 0) *bytes_send = l;
			return SOCKET_SEND_SEND_BUFFER_FULL;

		case SSL_ERROR_WANT_WRITE:

#if _WEBSERVER_CONNECTION_DEBUG_ > 3
			LOG(CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_WANT_WRITE","");
#endif
			if (bytes_send != 0) *bytes_send = l;
			return SOCKET_SEND_SEND_BUFFER_FULL;

		case SSL_ERROR_SSL:
		case SSL_ERROR_WANT_X509_LOOKUP:
		case SSL_ERROR_SYSCALL:
			err_code = ERR_get_error();
			if ( err_code == 0 ){
				if ( ( ret == 0 ) && ( errno == 0 )){
					return CLIENT_DISCONNECTED;
				}
				#if _WEBSERVER_CONNECTION_DEBUG_ > 1
					LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_SYSCALL ( %d / %d / %d / %m ) ",ret,r2, errno );
				#endif
			}else{
				ERR_error_string(err_code, buffer);
				#if _WEBSERVER_CONNECTION_DEBUG_ > 1
					LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket,"SSL_ERROR_SYSCALL ( %d / %d ) %s",ret,r2,buffer );
				#endif
			}
			return SSL_PROTOCOL_ERROR;

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
	#endif
	return SOCKET_SEND_SSL_ERROR;
}


#endif // WEBSERVER_USE_OPENSSL_CRYPTO
