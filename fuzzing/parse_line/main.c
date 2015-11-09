
#include <stdio.h>

#include <webserver.h>


int main( int argc, char** argv){

	char buf[10000];

	#ifdef AFL_PER
	while (__AFL_LOOP(10000)) {
	#endif
		memset(buf, 0, 10000);
		int len = read(0, buf, 10000);

		//printf("%X %X\n",buf[len-2],buf[len-1]);

		if ( len < 2 )
			#ifdef AFL_PER
			continue;
			#else
			return 0;
			#endif

		for ( int i = 2 ; i < len ; i++ ){

			if (( buf[i-2] == '\r') && ( buf[i-1] == '\n') ){

				len = i;
				buf[len] = '\0';
				//printf("%s\n",buf);

				//char* tmp = strtok( buf, "\r\n");
				//printf("%p %c\n",tmp);

				printf("   parsing\n");

				socket_info sock;
				HttpRequestHeader header;

				memset(&sock , 0, sizeof( socket_info ));
				memset(&header , 0, sizeof( HttpRequestHeader ));

				sock.header = &header;


				analyseHeaderLine( &sock, buf, len, &header );

				break;
			}
		}

	#ifdef AFL_PER
	}
	#endif


	return 0;
}
