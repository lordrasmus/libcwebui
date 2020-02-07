
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <webserver.h>


void print_header( HttpRequestHeader *header ){

	printf("-----------  Header Info Start  ----------------------------\n");

	if ( header->error == 0 )
		printf("Header Valid\n");
	else
		printf("Header Invalid\n");

	switch( header->method ){
		case HTTP_GET: printf("   Method: HTTP_GET\n"); break;
		case HTTP_POST: printf("   Method: HTTP_POST\n"); break;
	}

	if ( header->isHttp1_1 == 1 )
		printf("   Version: HTTP/1.1\n");
	else
		printf("   Version: HTTP/1.0\n");


	printf("   URL : %s\n",header->url);
	printf("   Content-Length: %ld\n",header->contentlenght);
	printf("   Host: %s\n",header->HostName);
	printf("   If-Modified-Since: %s\n",header->If_Modified_Since); /* Wed, 12 Dec 2007 13:13:08 GMT */
	printf("   If-None-Match: %s\n",header->etag);
	printf("   Upgrade: %s\n",header->Upgrade);
	printf("   Connection: %s\n",header->Connection);
	printf("   Origin: %s\n",header->Origin);
	printf("   Sec-WebSocket-Key: %s\n",header->SecWebSocketKey);
	printf("   Sec-WebSocket-Protocol: %s\n",header->SecWebSocketProtocol);
	printf("   Sec-WebSocket-Version: %d\n",header->SecWebSocketVersion);



	printf("Parameter :\n");
	ws_variable *var = getFirstVariable(header->parameter_store);
	while(var != 0){
		printf("  Name  : %s\n",var->name);
		printf("  Value : %s\n",var->val.value_string);
		var = getNextVariable(header->parameter_store);
	}

	printf("Cookies : \n");
	//printf("Count : %d\n",ws_list_size( &header->cookie_list ) );
	Cookie* cookie;
	ws_list_iterator_start(&header->cookie_list);
	while( ( cookie = (Cookie*)ws_list_iterator_next(&header->cookie_list) ) ){
		printf("  Name  : %s\n",cookie->name);
		printf("  Value : %s\n",cookie->value);
	}

	// Cookies
	// URL Parameter

	printf("-----------  Header Info End  ----------------------------\n");
}



// Eingentlich eine rein interne Funktione
void reCopyHeaderBuffer(socket_info* sock, unsigned int end) ;

int main( int argc, char** argv){

	
	

	socket_info sock;
	HttpRequestHeader *header = WebserverMallocHttpRequestHeader();

	memset(&sock , 0, sizeof( socket_info ));

	sock.header = header;
	sock.header_buffer = malloc( 10000 );
	sock.header_buffer_size = 10000;
	memset(sock.header_buffer, 0, 10000);
	
	int len;
	if ( argc == 1 ){
		len = read(0, sock.header_buffer, 10000);
	}else{
		struct stat st;
		stat(argv[1], &st);
		int size = st.st_size;
		int fd = open( argv[1], O_RDONLY );
		len = read(fd, sock.header_buffer, size);
	}

	sock.header_buffer_pos = len;

	unsigned int bytes_parsed;
	while(sock.header_buffer_pos>0){
		int len2 = ParseHeader( &sock, header, sock.header_buffer, sock.header_buffer_pos , &bytes_parsed );
		//printf("len: %d\n",len2);
		if (len2 > 0) {
			reCopyHeaderBuffer(&sock, bytes_parsed);
			continue;
		}
		break;
	}


	print_header( header );

	WebserverFreeHttpRequestHeader( header );


	return 0;
}
