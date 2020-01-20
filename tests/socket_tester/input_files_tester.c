
#include <sys/types.h>          /* See NOTES */
       #include <sys/socket.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>


int create_socket( void ){
	struct sockaddr_in sockaddr_in;
	in_addr_t in_addr;
 
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }
    
	//in_addr = inet_addr("192.168.11.98");
	in_addr = inet_addr("127.0.0.1");
    if (in_addr == (in_addr_t)-1) {
        //fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        exit(EXIT_FAILURE);
    }


	sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(8080);


 /* Do the actual connection. */
    if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect");
        exit( 1 );
    }    
    
    return sockfd;
}


void test_request( int sockfd, char* path, char* file ){
	
	char file_path[1000];
	
	static char buffer[100000];
	int offset = 0;
	
	sprintf( file_path, "%s%s",path, file );
	
	printf("Teste : %s\n",file_path);
	int fd = open( file_path, O_RDONLY );
    int l = read( fd, buffer, sizeof( buffer ) );
    close( fd );
    
    
    
    
    write( sockfd, buffer, l );
    
    printf("written : %d\n",l);
    memset( buffer, 0,  sizeof( buffer ) );
    
    return;
    
    offset = 0;
    int header = 0;
    while(1){
		int ret = read( sockfd, &buffer[offset], 1);
		if ( ret <= 0) break;
		offset += ret;
		
		if( ( buffer[0] == '\r' ) && ( buffer[1] == '\n' ) ){
			break;
		}
		
		if( ( buffer[offset-2] == '\r' ) && ( buffer[offset-1] == '\n' ) ){
			buffer[offset] = 0;
			//printf("%s",buffer);
			offset=0;
		}
		
		//printf("%c",buffer[0]);
		//fflush(stdout);
	}
}

void test_dir( char* path ){
	DIR *d;
    struct dirent *dir;
    
    int sock = create_socket();
    
    d = opendir( path );
    while ((dir = readdir(d)) != NULL)
	{
		if ( 0 == strcmp(dir->d_name, "." ) ) continue;
		if ( 0 == strcmp(dir->d_name, ".." ) ) continue;
		if ( 0 == strcmp(dir->d_name, "README.txt" ) ) continue;
		
		if ( 0 != strncmp(dir->d_name, "COOK", 4 ) ) continue;
		
		printf("%s\n", dir->d_name);
		
		test_request( sock, path, dir->d_name );
	}
	closedir(d);
}

int main(){

	
    
    test_dir( "../../fuzzing/parse_header/input/" );
    
    sleep( 1000 );
    
    exit(0);

    
    
    
}
