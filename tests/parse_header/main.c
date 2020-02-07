
#include <stdio.h>

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



int main( int argc, char** argv){

	char buf[10000];


	memset(buf, 0, 10000);
	int len = read(0, buf, 10000);


	socket_info sock;
	HttpRequestHeader *header = WebserverMallocHttpRequestHeader();

	memset(&sock , 0, sizeof( socket_info ));

	sock.header = header;

	unsigned int bytes_parsed;
	ParseHeader( &sock, header, buf, len , &bytes_parsed );


	print_header( header );

	WebserverFreeHttpRequestHeader( header );


	return 0;
}
