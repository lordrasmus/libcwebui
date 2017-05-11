
#include <stdio.h>

#include <webserver.h>

extern "C" void deleteEvent(socket_info* sock){}
extern "C" unsigned long dumpSocketsSize(int *count) { return *count; }
extern "C" void dumpSockets(http_request* s) { }
extern "C" char isTemplateFile(const char* file) { return 0; }


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {

	socket_info *sock = malloc(sizeof( socket_info));
	HttpRequestHeader *header = WebserverMallocHttpRequestHeader();
	
	char *buffer = malloc( Size*2 );
	memcpy( buffer, Data, Size ); // Input data is modified

	memset(&sock , 0, sizeof( socket_info ));

	sock->header = header;

	unsigned int bytes_parsed;
	ParseHeader( sock, header, buffer, Size , &bytes_parsed );

	WebserverFreeHttpRequestHeader( header );

	free( buffer );
	free( sock );

	return 0;
}
