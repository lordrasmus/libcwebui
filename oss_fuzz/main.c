
#include <stdio.h>

#include <webserver.h>


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

	socket_info sock;
	HttpRequestHeader *header = WebserverMallocHttpRequestHeader();
	
	char *buffer = (char*)malloc( Size );
	memcpy( buffer, Data, Size ); // Input data is modified

	memset(&sock , 0, sizeof( socket_info ));

	sock.header = header;

	unsigned int bytes_parsed;
	ParseHeader( &sock, header, buffer, Size , &bytes_parsed );

	WebserverFreeHttpRequestHeader( header );

	free( buffer );

	return 0;
}
