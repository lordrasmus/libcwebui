#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

char buffer_v12[] = {"GET ws://localhost:8080/TestSocket HTTP/1.1\r\n"
	"Host: localhost:8080\r\n"
	"Connection: Upgrade\r\n"
	"Pragma: no-cache\r\n"
	"Cache-Control: no-cache\r\n"
	"Upgrade: websocket\r\n"
	"Origin: http://localhost:8080\r\n"
	"Sec-WebSocket-Version: 12\r\n"
	"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36\r\n"
	"Accept-Encoding: gzip, deflate, br\r\n"
	"Accept-Language: de-DE,de;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
	"Sec-WebSocket-Key: X1INsmyllnbERItpz6vgSg==\r\n"
	"Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n\r\n"};

char buffer_v13[] = {"GET ws://localhost:8080/TestSocket HTTP/1.1\r\n"
	"Host: localhost:8080\r\n"
	"Connection: Upgrade\r\n"
	"Pragma: no-cache\r\n"
	"Cache-Control: no-cache\r\n"
	"Upgrade: websocket\r\n"
	"Origin: http://localhost:8080\r\n"
	"Sec-WebSocket-Version: 13\r\n"
	"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36\r\n"
	"Accept-Encoding: gzip, deflate, br\r\n"
	"Accept-Language: de-DE,de;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
	"Sec-WebSocket-Key: X1INsmyllnbERItpz6vgSg==\r\n"
	"Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n\r\n"};
	
char buffer_methode[] = {"DUMMY http://localhost:8080/TestSocket HTTP/1.1\r\n"
	"Host: localhost:8080\r\n\r\n"
	"\r\n"};

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 

    
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080); 

    if(inet_pton(AF_INET, "10.100.2.14", &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
    
    printf("send\n");
    
    char *buffer= buffer_v13;;
    if ( argc > 1 ){
		
		switch( atoi( argv[1] )){
			default : buffer = buffer_v13; break;
			case 1: buffer = buffer_v12; break;
			case 2: buffer = buffer_methode; break;
		}
	}


	//int len = write(sockfd,buffer,strlen(buffer));
	int len = strlen(buffer);
	int send = 0;
	while(1){
		//char c = getchar( );
		
		send += write(sockfd,&buffer[send],1);
		printf("send: %d\n",send);
		
		//sleep(10);
		if ( send == len ) 
			break;
	}
	
	printf("bytes %d\n",len);
	
    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 

    return 0;
}
