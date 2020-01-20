
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



int main(){

	int sock = create_socket();
	
	char* host_and_port = "127.0.0.1:4443"; 
    char* server_request_template = "GET /mini_template.html HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"; 
	
	char server_request[5000];
	memset( server_request, 0 , sizeof( server_request ) );
	
	while( strlen( server_request ) < 2500 ){
		strcat( server_request, server_request_template );
	}
	
	int single_write = 0;
	int enters = 0;
	
	if ( single_write == 1 ){
		write( sock, server_request, strlen( server_request) );
	}else{
		int off = 0;
		while( off < strlen( server_request) ){
			// 70 ist einer komplett und einer teilweise
			off += write( sock, &server_request[off], 65 );
			//sleep( 20 );
			getc(stdin);
			enters++;
			printf("enters: %d\n",enters);
			fflush(stdout);
		}
	}
    
    sleep( 1000 );
    
    exit(0);

    
    
    
}
