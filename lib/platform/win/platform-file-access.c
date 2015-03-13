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

//#include "system.h"


FILE	*g_fp;
unsigned short l;

void	ErrorExit(LPTSTR lpszFunction);

/**************************************************************
*                                                             *
*                   Datei Operationen                         *
*                                                             *
**************************************************************/



char	PlatformOpenDataReadStream( char* name )
{   
	errno_t ret;
/*	char d[1000];
#ifndef WEBSERVER_USE_BINARY_FORMAT
	strcpy_s(d,1000,file_dir);
	strcat_s(d,1000,name);
#else
	strcpy_s(d,1000,name);
#endif
*/	
	ret = fopen_s( &g_fp,name,"rb"); 
	if(ret == 0)
		return true;
	else
		return false;
}


int     PlatformGetFileSize(void){
	int size;
	fseek (g_fp , 0 , SEEK_END);
	size = ftell (g_fp);
	fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
	return size;
}


char	PlatformCloseDataStream( void )
{
    fclose(g_fp);
	return 0;
}

void	PlatformResetDataStream( void )
{
    fseek(g_fp,0,SEEK_SET);		// Anfang der Datei
}

int PlatformReadBytes(unsigned char *data, unsigned int lenght)
{
    return fread(data,lenght,1,g_fp);
}

void	PlatformSeek(long offset){
	fseek(g_fp, offset,SEEK_CUR);
}

void	PlatformSeekToPosition( long position )
{
    fseek(g_fp,position,SEEK_SET);
}

unsigned long PlatformGetDataStreamPosition()
{
    return ftell(g_fp);
}


char	PlatformGetFileTime(WebserverFileInfo* file){
	FILETIME cr,la,ftWrite;
	SYSTEMTIME stUTC, stLocal;
    DWORD dwRet;
	HANDLE fh;
	char bb[1000];
	
	strcpy_s(bb,1000,file->FilePath);
	//strcat_s(bb,1000,file->Name);

	fh = CreateFileA((LPCSTR)bb,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	
	if ( fh == INVALID_HANDLE_VALUE ){
		//dwRet = GetLastError();
		ErrorExit(TEXT("WebserverGetFileTime"));
		CloseHandle(fh);
		return false;
	}

	if(!GetFileTime(fh,&cr,&la,&ftWrite) ) {
		CloseHandle(fh);
		return false;
	}

	CloseHandle(fh);
	
    FileTimeToSystemTime(&ftWrite, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	dwRet = getHTMLDateFormat(bb,stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute);
    
	if(file->lastmodified != 0){
		if(0==strcmp(bb,file->lastmodified)) return true;
		WebserverFree(file->lastmodified);
		file->lastmodified=0;
	}
	file->lastmodifiedLength = (unsigned char)dwRet;
	if(file->lastmodified != 0)
		WebserverFree(file->lastmodified);
	file->lastmodified = WebserverMalloc(dwRet+1,1);

	strcpy_s(file->lastmodified,file->lastmodifiedLength+1,bb);
	file->lastmodified[file->lastmodifiedLength] = '\0';


	return false;

}





void	ErrorExit(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    //ExitProcess(dw); 
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

