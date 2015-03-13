/*

libCWebUI
Copyright (C) 2012  Ramin Seyed-Moussavi

Projekt URL : http://code.google.com/p/libcwebui/

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <linux/types.h>
//#include <linux/netdevice.h>


#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <linux/if.h>
#include <net/if.h>


#include "platform.h"
//#include "../platform/include/platfom.h"
//#include "../include/platfom.h"




/*#include <sys/ioctl.h>
//#include <sys/ioctl.h>
//#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
//#include <net/if.h>
#include <linux/if.h>
#include <sys/time.h>
*/
//struct ifreq




/*FILE	*g_fp;
struct sockaddr_in g_serv_addr, g_cli_addr,g_serv_addr_ssl, g_cli_addr_ssl;
int		g_socketfd,g_socketfd_ssl;
unsigned int g_clilen,g_clilen_ssl;

//int accept_socket;
int g_use_ssl;
*/


/**************************************************************
*                                                             *
*                   Speicher Verwaltung                       *
*                                                             *
**************************************************************/
//#define MALLOC_DEBUG
#ifdef MALLOC_DEBUG
static int mallocs;
#endif
void*	PlatformMalloc(  int size)
{
	#ifdef MALLOC_DEBUG
	unsigned char *p;
	p = malloc(size+2);
	p[0]=size;
	p[1]=size>>8;
	printf("Malloc %X (%d) %d\r\n",p+2,size,++mallocs);
	return p+2;
	#else
	return malloc(size);
	#endif
}

void	PlatformFree( void *mem )
{
	#ifdef MALLOC_DEBUG
	unsigned char *p=mem-2;
	int l = p[0] + (p[1]<<8);
	printf("Free %X (%d) %d\r\n",mem,l,mallocs--);
    free(p);
	#else
	free(mem);
	#endif
}

void	WebserverPrintMemInfo( void ){}			// gibt auf dem DSTni verfuegbaren Arbeitspeicher aus








/*********************************************************************

					System informationen

*********************************************************************/



void PlatformGetNetmask(unsigned char* bytes){
	bytes[0]=255;
	bytes[1]=255;
	bytes[2]=255;
	bytes[3]=0;
}

void PlatformGetMAC(unsigned char* bytes){
	bytes[0]=0;
	bytes[1]=0;
	bytes[2]=2;
	bytes[3]=5;
	bytes[4]=5;
	bytes[5]=5;
}

void PlatformGetIP(unsigned char* bytes){
  struct ifreq ifa;
  struct sockaddr_in *i;
  int fd;

  strcpy (ifa.ifr_name, "eth0");

  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror ("socket");
    exit (EXIT_FAILURE);
  }

  if(ioctl(fd, SIOCGIFADDR, &ifa)) {
    perror ("ioctl");
    exit (EXIT_FAILURE);
  }

  //if( ioctl( sock_fd , SIOCGIFHWADDR , &ifr ) == -1 ) { printf("problems with ioctl.\n"); exit(1); }
  //printf("%s\n" , ether_ntoa( (struct ether_addr*) ifr.ifr_hwaddr.sa_data ) );

  i = (struct sockaddr_in*)&ifa.ifr_addr;
  close(fd);
  bytes[3] = (i->sin_addr.s_addr >> 24) & 0xff;
  bytes[2] = (i->sin_addr.s_addr >> 16) & 0xff;
  bytes[1] = (i->sin_addr.s_addr >> 8) & 0xff;
  bytes[0] = i->sin_addr.s_addr & 0xff;

  /*bytes[3] = 78;
  bytes[2] = 2;
  bytes[1] = 168;
  bytes[0] = 192;*/
}

unsigned long long PlatformGetTick(void){
	return time(0);
}

unsigned long PlatformGetTicksPerSeconde(void){
	return 1000; // Konstante HZ benutzen ??
}

#ifdef WEBSERVER_USE_SESSIONS
static 	unsigned int guid;
void 	PlatformGetGUID( char* buf,int length){
	int l=0;
		l = snprintf((char*)buf,length,"\"Test %d",guid++);
		for(;l<length;l++){
			buf[l]='+';
		}
		buf[length-2]='\"';
		buf[length-1]='\0';
}
#endif

// http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
// http://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html

#ifdef jojo



/************************************************
*																								*
*   Thread Funktionen 													*
*																								*
************************************************/

void    startWebServer( WebserverConfig* config )
{
    int_startWebServer(config);
}

bool  startWebserverTask( WebserverConfig* config )
{
    /*int ret;
    pthread_t thread1;
    ret = pthread_create( &thread1, NULL, int_webserverTask, (void*)&sockfd); // (void *()(void*))
    if (ret==0)
        return true;
    //runtsk(250,(void(*)(void))int_webserverTask,4000,newsockfd);
    if(ret == 12){
        printf("pthread error %d \nrunnig as non thread",ret);
        int_webserverTask(sockfd);
        return false;
    }
    printf("pthread error %d \n",ret);
	return false;*/
	//socket_info info;
	//info.socket = sockfd;
	#ifdef WEBSERVER_USE_SSL
	//info.use_ssl = g_use_ssl;
	//info.sbio=BIO_new_socket(info.socket,BIO_NOCLOSE);
	//info.ssl=SSL_new(ctx);
	//SSL_set_bio(info.ssl,info.sbio,info.sbio);
	#endif
	int_webserverTask(config);
    return true;
}



int PlatformCreateSocket(void){
	return socket(AF_INET, SOCK_STREAM, 0); // SO_REUSEADDR
}

bool PlatformConnectSocket(int socket,unsigned char* ip,int port){
	struct sockaddr_in dstAddr;
	memset(&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
    	memcpy(&dstAddr.sin_addr, ip, 4);
   	dstAddr.sin_port = htons(port);
	connect(socket, (struct sockaddr *)&dstAddr, sizeof(dstAddr));
	return true;
}

void PlatformCloseListenSockets(void){
	close(g_socketfd);
	close(g_socketfd_ssl);
}

int WebserverGetSocket( short port,int ssl_sock )
{
	int sock = PlatformCreateSocket();

    if (sock == -1)
    {
        WebServerPrintf("socket() failed");
        return sock;
    }

	if(ssl_sock==1){
		g_socketfd_ssl = sock;
		g_serv_addr_ssl.sin_family = AF_INET;
		//serv_addr.sin_addr.s_addr = INADDR_ANY;
		g_serv_addr_ssl.sin_port = htons(port);
		if (bind(g_socketfd_ssl, (struct sockaddr *) &g_serv_addr_ssl,sizeof(g_serv_addr_ssl)) < 0)
		{
			WebServerPrintf("ERROR on binding Port %d\r\n",port);
			return -1;
		}
		listen(g_socketfd_ssl,5);
		g_clilen_ssl = sizeof(g_cli_addr_ssl);
		return g_socketfd_ssl;
	}else{
		g_socketfd = sock;
		g_serv_addr.sin_family = AF_INET;
		//serv_addr.sin_addr.s_addr = INADDR_ANY;
		g_serv_addr.sin_port = htons(port);
		if (bind(g_socketfd, (struct sockaddr *) &g_serv_addr,sizeof(g_serv_addr)) < 0)
		{
			WebServerPrintf("ERROR on binding Port %d\r\n",port);
			return -1;
		}
		listen(g_socketfd,5);
		g_clilen = sizeof(g_cli_addr);
		return g_socketfd;
	}

}



int		WebserverAccept( void )
{
	int ret;
	struct timeval timeout;
	fd_set socks;        // Socket file descriptors we want to wake up for, using select()
	timeout.tv_sec = 60;
	FD_ZERO(&socks);

	FD_SET(g_socketfd,&socks);

	#ifdef WEBSERVER_USE_SSL
	FD_SET(g_socketfd_ssl,&socks);
	ret = select(g_socketfd_ssl+1, &socks, (fd_set *) 0, (fd_set *) 0, 0);
	#else
	ret = select(g_socketfd+1, &socks, (fd_set *) 0, (fd_set *) 0, 0);
	#endif

	if( ret == -1){
		printf("SELECT Error1\r\n");
		//exit(0);
		return 0;
	}
	//printf("%d %d\r\n",g_socketfd,g_socketfd_ssl);
	//printf("Select %d\r\n",ret);

	#ifdef WEBSERVER_USE_SSL
	if(FD_ISSET(g_socketfd_ssl, &socks)	!= 0){
		#ifdef _WEBSERVER_DEBUG_
		printf("SSL Connection\r\n");
		#endif
		g_use_ssl = 1;
		return accept(g_socketfd_ssl,(struct sockaddr *) &g_cli_addr_ssl,&g_clilen_ssl);
	}
	#endif

	if(FD_ISSET(g_socketfd, &socks)	!= 0){
		#ifdef _WEBSERVER_DEBUG_
		printf("Normal Connection\r\n");
		#endif
		#ifdef WEBSERVER_USE_SSL
		g_use_ssl = 0;
		#endif
		return accept(g_socketfd,(struct sockaddr *) &g_cli_addr,&g_clilen);
	}

	printf("SELECT Error2\r\n");
	return 0;
}


int     WebserverPlatformSend(socket_info* s, unsigned char *buf, int len, int flags)
{
    return send(s->socket,buf,len,flags);
}

int     WebserverPlatformRecv(socket_info* s, unsigned char *buf, int len, int flags)
{
    return recv(s->socket,buf,len,flags);
}

int     WebserverPlatformCloseSocket(socket_info* socket)
{
    return close(socket->socket);
}

#endif



