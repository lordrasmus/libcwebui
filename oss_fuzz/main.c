
#include <stdio.h>

#include <webserver.h>


int main( int argc, char** argv){

	char buf[10000];

	#ifdef AFL_PER
	while (__AFL_LOOP(10000)) {
	#endif
		memset(buf, 0, 10000);
		int len = read(0, buf, 10000);


		socket_info sock;
		HttpRequestHeader *header = WebserverMallocHttpRequestHeader();

		memset(&sock , 0, sizeof( socket_info ));

		sock.header = header;

		unsigned int bytes_parsed;
		ParseHeader( &sock, header, buf, len , &bytes_parsed );

		WebserverFreeHttpRequestHeader( header );

	#ifdef AFL_PER
	}
	#endif


	return 0;
}
