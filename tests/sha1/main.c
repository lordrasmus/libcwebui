
#include <stdio.h>

#include <openssl/sha.h>

#include "webserver.h"



// just a stub
void handleer( int a, short b, void *t ) { }


void test_sha1( unsigned char *data, int l ){
	unsigned char hash_my[SSL_SHA_DIG_LEN];
	unsigned char hash_openssl[SSL_SHA_DIG_LEN];
	
	int i;
	
	printf("\ndata     = ");
    for(i=0;i<l;i++)
        printf("%02x", data[i]);
    printf("\n");
    
	
	WebserverSHA1( data, l, hash_my );
	
	printf("my  SHA1 = ");
    for(i=0;i<SSL_SHA_DIG_LEN;i++)
        printf("%02x", hash_my[i]);
    printf("\n");
    
	SHA1(data, l, hash_openssl);

    printf("ssl SHA1 = ");
    for(i=0;i<SSL_SHA_DIG_LEN;i++)
        printf("%02x", hash_openssl[i]);
    printf("\n");
    
    int ok = 1;
    for(i=0;i<SSL_SHA_DIG_LEN;i++){
		if ( hash_openssl[i] != hash_my[i] ){
			ok = 0;
			break;
		}
	}
	
	if ( ok == 1 ){
		printf("passed\n");
	}else{
		printf("erro\n");
	}
    
    
}

int main(){
	
	printf("\nRunning sha1 test ...\n\n");
	
	unsigned char data1[]={ "abc"};
	test_sha1( data1, sizeof( data1 ) -1 );
    
    unsigned char data2[]={ "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
	test_sha1( data2, sizeof( data2 ) -1);
	
	unsigned char data3[]={ "a"};
	test_sha1( data3, sizeof( data3 ) -1);
}
