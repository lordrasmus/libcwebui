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


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
 #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>



#include "platform.h"

#include "system.h"
#include "utils.h"



unsigned short l;

#ifdef WEBSERVER_USE_BINARY_FORMAT
unsigned long data_pos;
extern char webserver_data[];
#else
FILE	*g_fp;
#endif


/**************************************************************
*                                                             *
*                   Datei Operationen                         *
*                                                             *
**************************************************************/



bool	PlatformOpenDataReadStream( char* name )
{   
	#ifndef WEBSERVER_USE_BINARY_FORMAT
	struct stat st;	
	char d[1000];
	int fd;	
	strcpy(d,file_dir);
	strcat(d,(char*)name);
	strcat(d,(char*)"\0");
	
	g_fp = fopen(d,"r");	
	if(g_fp==NULL)
		return false;
	#else
	data_pos = 0;
	#endif	
	
	return true;
}


int     PlatformGetFileSize(void){
	//struct stat st;
	#ifndef WEBSERVER_USE_BINARY_FORMAT
	int size;
	fseek (g_fp , 0 , SEEK_END);
	size = ftell (g_fp);
	fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
	//fstat(g_fp, &st);
	return size;
	#else
	return 0;
	#endif
}


bool	PlatformCloseDataStream( void )
{
	#ifndef WEBSERVER_USE_BINARY_FORMAT
    fclose(g_fp);
    #endif
	return true;
}

void	PlatformResetDataStream( void )
{
	#ifndef WEBSERVER_USE_BINARY_FORMAT
    fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
    #else
    data_pos =0;
    #endif
}


void	PlatformReadBytes(unsigned char *data,unsigned int length)
{
	#ifndef WEBSERVER_USE_BINARY_FORMAT
    fread(data,length,1,g_fp);
    #else
    memcpy(data,&webserver_data[data_pos],length);
    data_pos+=length;
    #endif
}

void	PlatformSeek(long offset){
	#ifndef WEBSERVER_USE_BINARY_FORMAT
	fseek(g_fp, offset,SEEK_CUR);
	#else
	data_pos+=offset;
	#endif
}

void	PlatformSeekToPosition( long position )
{
	#ifndef WEBSERVER_USE_BINARY_FORMAT
    fseek(g_fp,position,SEEK_SET);
    #else
    data_pos = position;
    #endif
}

unsigned long PlatformGetDataStreamPosition()
{
	#ifndef WEBSERVER_USE_BINARY_FORMAT
    return ftell(g_fp);
    #else
    return data_pos;
    #endif
}

/*	 struct stat {
	             off_t     st_size;    // total size, in bytes
	              time_t    st_atime;   // time of last access
	              time_t    st_mtime;   // time of last modification
	              time_t    st_ctime;   // time of last status change
	          };
	 */

bool PlatformGetFileTime(WebserverFileInfo* file){

	struct stat st;
	struct tm  *ts;
	int len;
	char* buffer = (char *)WebserverMalloc(150,0);

	stat(file->Name, &st);

	ts = localtime(&st.st_mtime);
	len = getHTMLDateFormat(buffer,ts->tm_mday,ts->tm_mon,ts->tm_year+1900,ts->tm_hour,ts->tm_min);

	if(file->lastmodified == 0){
		file->lastmodified = (char *)WebserverMalloc(len+1,0);
	}else{
		WebserverFree(file->lastmodified);
		file->lastmodified = (char *)WebserverMalloc(len+1,0);
	}
	memcpy(file->lastmodified,buffer,len+1);
	file->lastmodifiedLength = len;

	if(file->etag == 0){
		file->etag = (char *)WebserverMalloc(100,0);
	}
	file->etagLength = sprintf((char*)file->etag,"Test %s",file->Name);
	file->etagLength = 100;


	WebserverFree(buffer);
	return false;
}



#ifdef jo


bool	WebserverOpenDataReadStream( void )
{   
	errno_t ret = fopen_s( &g_fp,"data.bin","rb"); 
	if(ret == 0)
		return true;
	else
		return false;
}
bool	WebserverOpenDataWriteStream2(  char* name )
{    
	errno_t ret = fopen_s( &g_fp,name,"wb"); 
	if(ret == 0)
		return true;
	else
		return false;
}

bool	WebserverOpenDataWriteStream( void )
{    
	errno_t ret = fopen_s( &g_fp,"data.bin","wb"); 
	if(ret == 0)
		return true;
	else
		return false;
}

void	WebserverWriteBytes(unsigned char *data,unsigned int lenght)
{
    fwrite(data,lenght,1,g_fp);
}
#endif

#ifdef blkj



/**************************************************************
*                                                             *
*                   Datei Operationen                         *
*                                                             *
**************************************************************/

bool    WebserverOpenDataReadStream( void )
{
    g_fp = fopen("data.bin","rb");
    if(g_fp==NULL)
        return false;
    return true;
}

bool	WebserverOpenDataReadStream2( unsigned char* name ){
	char d[1000];
	strcpy(d,file_dir);
	strcat(d,(char*)name);
	printf("\n%s\n",d);
	g_fp = fopen(d,"rb");
    if(g_fp==NULL)
        return false;
    return true;
}

bool	WebserverOpenDataWriteStream( void )
{
    g_fp = fopen("data.bin","wb");
    if(g_fp==NULL)
        return false;
    return true;
}

bool	WebserverOpenDataWriteStream2( unsigned char* name )
{
    g_fp = fopen((char*)name,"wb");
    if(g_fp==NULL)
        return false;
    return true;
}

int     WebserverGetFileSize(void){
	int size;
	fseek (g_fp , 0 , SEEK_END);
	size = ftell (g_fp);
	fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
	return size;
}

bool	WebserverCloseDataStream( void )
{
    int ret = fclose(g_fp);
    if(ret==0)
        return true;
    else
        return false;
}

void	WebserverResetDataStream( void )
{
    fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
}

void	WebserverReadBytes(unsigned char *data,unsigned int lenght)
{
    fread(data,lenght,1,g_fp);
}

void	WebserverWriteBytes(unsigned char *data,unsigned int lenght)
{
    fwrite(data,lenght,1,g_fp);
}

void WebserverSeek(long offset){
	fseek(g_fp, offset,SEEK_CUR);
}

void WebserverSeekToPosition( long position )
{
    fseek(g_fp,position,SEEK_SET);
}

unsigned long WebserverGetDataStreamPosition()
{
    return ftell(g_fp);
}



#endif


#ifdef blalal


/**************************************************************
*                                                             *
*                   Datei Operationen                         *
*                                                             *
**************************************************************/


bool    WebserverOpenDataReadStream( void )
{
/*    g_fp = fopen("data.bin","rb");
    if(g_fp==NULL)
        return false;
    return true;
    */
    
    char filename[]="FLASH0/data.bin";
  
  	//int  permissions = NASYSACC_FS_GROUP2_READ;
  	int  mode = O_RDONLY;
  	
//	g_fp = open(filename, mode, permissions);	
	g_fp = fopen(filename, "r");	
    if (g_fp == 0)
    {
        printf("WebserverOpenDataReadStream Error: opened \"%s\" with mode 0x%X (%d errno)\n", filename, mode, getErrno());      
        return false;  
    }    
    return true;
}

bool	WebserverOpenDataWriteStream( void )
{
   /* g_fp = fopen("data.bin","wb");
    if(g_fp==NULL)
        return false;
    return true;*/
    	printf("WebserverOpenDataWriteStream Noch nicht implementiert\n");
        #warning "WebserverOpenDataWriteStream Noch nicht implementiert"
        return false;
}

bool	WebserverCloseDataStream( void )
{
	
    int ret = fclose(g_fp);
    if(ret==0)
        return true;
    else
        return false;
        
      
   //#warning "Noch nicht implementiert"
}

void	WebserverResetDataStream( void )
{
    fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
   // #warning "Noch nicht implementiert"
}

void	WebserverReadBytes(unsigned char *data,unsigned int lenght)
{
    fread(data,lenght,1,g_fp);
//    #warning "Noch nicht implementiert"
}

void	WebserverWriteBytes(unsigned char *data,unsigned int lenght)
{
	  fwrite(data,lenght,1,g_fp);
    //  #warning "Noch nicht implementiert"
}

void WebserverSeek(long offset){
	fseek(g_fp, offset,SEEK_CUR);
//    #warning "Noch nicht implementiert"
}

void WebserverSeekToPosition( long position )
{
    fseek(g_fp,position,SEEK_SET);
//    #warning "Noch nicht implementiert"
}

unsigned long WebserverGetDataStreamPosition()
{
    return ftell(g_fp);
//    #warning "Noch nicht implementiert"
}

#endif
