
#include <stdio.h>

#include <openssl/sha.h>

#include "webserver.h"

 #include <openssl/buffer.h>
 #include <openssl/bio.h>
 #include <openssl/evp.h>

// just a stub
void handleer( int a, short b, void *t ) { }

void SSLBase64Encode(const unsigned char *input, int length, unsigned char *output, SIZE_TYPE out_length) {
	BIO *bmem, *b64;
	BUF_MEM *bptr;
	int ret;
	
	memset( output, 0 , out_length );

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64,BIO_FLAGS_BASE64_NO_NL);
	
	bmem = BIO_new(BIO_s_mem());
	BIO_set_flags( bmem, BIO_FLAGS_BASE64_NO_NL );
	
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	// ret gibts nur wegen compiler warnings
	ret = BIO_flush(b64);
	ret++;
	BIO_get_mem_ptr(b64, &bptr);

	if (bptr->length < out_length) {
		memcpy(output, bptr->data, bptr->length - 0);
	} else {
		printf("WebserverBase64 ausgabe buffer zu klein\n");
	}

	BIO_free_all(b64);
}

void test_base64( unsigned char *data, int l ){
	unsigned char hash_my[200];
	unsigned char hash_openssl[200];


	int i;
	
	printf("\ndata       = %s \n             ",data);
    for(i=0;i<l;i++)
        printf("%02x", data[i]);
    printf("\n");
    

	WebserverBase64Encode( data, l, hash_my , 200 );
	
	printf("my  base64 = %s\n",hash_my);

	SSLBase64Encode(data, l, hash_openssl, 200);

    printf("ssl base64 = %s\n",hash_openssl);
    
    int ok = 1;
    for(i=0;i<strlen(hash_my) ;i++){
		if ( hash_openssl[i] != hash_my[i] ){
			ok = 0;
			break;
		}
	}
	
	if ( ok == 1 ){
		printf("passed\n");
	}else{
		printf("error\n");
	}
    

}

int main(){
	
	printf("\nRunning base64 test ...\n\n");
	
	unsigned char data1[]={ "abc"};
	test_base64( data1, sizeof( data1 ) -1 );
    
    unsigned char data2[]={ "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
	test_base64( data2, sizeof( data2 ) -1);
	
	unsigned char data3[]={ "a"};
	test_base64( data3, sizeof( data3 ) -1);
}
