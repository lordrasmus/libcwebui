/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



//#include "bsp.h"
#include "sockapi.h"
#include "tx_api.h"

#include "fs.h"
#include "netosIo.h"
#include "bsp_api.h"
#include "sysAccess.h"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

 
#include "system.h"




//FILE	*g_fp;
//int g_fp;
/*struct sockaddr_in g_serv_addr, g_cli_addr;
int		g_socketfd;
unsigned int g_clilen;
*/


/**************************************************************
*                                                             *
*                   Speicher Verwaltung                       *
*                                                             *
**************************************************************/


void*	PlatformMalloc(  int size)
{
    return malloc(size);
//    #warning "Noch nicht implementiert"
}

void	PlatformFree( void *mem )
{
    free(mem);
//    #warning "Noch nicht implementiert"
}
void	WebserverPrintMemInfo( void ){}			// gibt auf dem DSTni verfügbaren Arbeitspeicher aus



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
  bytes[3] = 74;
  bytes[2] = 2;
  bytes[1] = 168;
  bytes[0] = 192;
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




