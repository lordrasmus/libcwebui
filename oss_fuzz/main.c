
#include <stdio.h>

#include <webserver.h>

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

	socket_info sock;
	HttpRequestHeader *header = WebserverMallocHttpRequestHeader();

	memset(&sock , 0, sizeof( socket_info ));

	sock.header = header;

	unsigned int bytes_parsed;
	ParseHeader( &sock, header, Data, Size , &bytes_parsed );

	WebserverFreeHttpRequestHeader( header );


	return 0;
}
