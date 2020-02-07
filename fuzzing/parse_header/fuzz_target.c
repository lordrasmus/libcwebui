
#include <stdio.h>

#include <webserver.h>

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	socket_info sock;
	HttpRequestHeader *header = WebserverMallocHttpRequestHeader();
	char *buf =malloc( Size );

	memcpy( buf, Data, Size );
	memset(&sock , 0, sizeof( socket_info ));

	sock.header = header;

	unsigned int bytes_parsed;
	ParseHeader( &sock, header, buf, Size , &bytes_parsed );

	WebserverFreeHttpRequestHeader( header );
	
	free( buf );
	
	return 0;  // Non-zero return values are reserved for future use.
}
