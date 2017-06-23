
#include <stdio.h>

#include <webserver.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

extern unsigned long allocated;

int main(int argc, char** argv) {

	if ( argc < 2 ){
		printf("input datei angeben\n");
		exit(0);
	}

	printf("%d\n",argc);
	printf("%s\n",argv[1]);
	
	
	    for ( int i = 1 ; i < argc; i++ ){

	    struct stat fileStat;
	    if(stat(argv[1],&fileStat) < 0) {
			printf("Error beim stats lesen von %s : %m\n");
		return 1;
		}
		
	    printf("\nTesting file ...\n");
	    printf("Information for %s\n",argv[1]);
	    printf("---------------------------\n");
	    printf("File Size: \t\t%d bytes\n",fileStat.st_size);
	    //printf("Number of Links: \t%d\n",fileStat.st_nlink);
	    
	    char *buffer = (char*)malloc( fileStat.st_size );
	    
	    FILE* fp = fopen(argv[i],"r");
	    fread( buffer, fileStat.st_size , 1 , fp );
	    fclose(fp);



	    socket_info sock;
	    HttpRequestHeader *header = WebserverMallocHttpRequestHeader();
	    
	    memset(&sock , 0, sizeof( socket_info ));

	    sock.header = header;

	    unsigned int bytes_parsed;
	    ParseHeader( &sock, header, buffer, fileStat.st_size , &bytes_parsed );

	    WebserverFreeHttpRequestHeader( header );

	    free( buffer );
	}
	
	printf("allocated : %d\n",allocated);

	return 0;
}
