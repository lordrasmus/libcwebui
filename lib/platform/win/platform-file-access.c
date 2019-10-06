/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/




#include <stdio.h>

#include "webserver.h"

#pragma comment(lib, "Ws2_32.lib")

FILE	*g_fp;
unsigned short l;

void	ErrorExit(LPTSTR lpszFunction);

/**************************************************************
*                                                             *
*                   Datei Operationen                         *
*                                                             *
**************************************************************/


int PlatformOpenDataReadStream(const char* name) {

#ifdef _MSC_VER
	errno_t ret;
    ret = fopen_s( &g_fp,name,"rb"); 
    if (ret != 0)
		return false;
#else
    g_fp = fopen( name,"rb"); 
    if( g_fp == 0 )
        return false;
#endif

	return _fileno(g_fp);
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

int PlatformReadBytes(unsigned char *data, FILE_OFFSET lenght)
{
    return fread(data,1,lenght,g_fp);
}

void	PlatformSeek(long offset){
	fseek(g_fp, offset,SEEK_CUR);
}

void	PlatformSeekToPosition( long position )
{
    fseek(g_fp,position,SEEK_SET);
}

unsigned long PlatformGetDataStreamPosition( void )
{
    return ftell(g_fp);
}


int PlatformGetFileInfo(WebserverFileInfo* file, int* time_changed, int *new_size){
	FILETIME cr,la,ftWrite;
	SYSTEMTIME stUTC, stLocal;
    HANDLE fh;
	char buffer[1000];

	unsigned long int f_sec;
	unsigned long int f_nsec;
	
	size_t len;

#ifdef _MSC_VER
	strcpy_s(buffer,1000,file->FilePath);
#else
    strncpy(buffer,file->FilePath,1000);
#endif
	//strcat_s(bb,1000,file->Name);

	fh = CreateFileA((LPCSTR)buffer,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	
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

	DWORD size = GetFileSize(fh, NULL );

	CloseHandle(fh);
	
    FileTimeToSystemTime(&ftWrite, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	*new_size = size;
	*time_changed = 0;
		
	// Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
	static const uint64_t EPOCH_DIFFERENCE_MICROS = 11644473600000000ull;

	// First convert 100-ns intervals to microseconds, then adjust for the
	// epoch difference
	uint64_t total_us = (((uint64_t)ftWrite.dwHighDateTime << 32) | (uint64_t)ftWrite.dwLowDateTime) / 10;
	total_us -= EPOCH_DIFFERENCE_MICROS;

	// Convert to (seconds, microseconds)
	f_sec = (unsigned long int)(total_us / 1000000);
	f_nsec = (unsigned long int)(total_us % 1000000);


	if ((file->last_mod_sec != (unsigned long int)f_sec) || (file->last_mod_nsec != (unsigned long int)f_nsec)) {

		*time_changed = 1;

		file->last_mod_sec = f_sec;
		file->last_mod_nsec = f_nsec;

		len = getHTMLDateFormat(buffer, stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute);

		if (file->lastmodified == 0) {
			file->lastmodified = (char *)WebserverMalloc(len + 1);
			memcpy(file->lastmodified, buffer, len + 1);
			file->lastmodifiedLength = len;
			return true;
		}
		else {
			if (0 != strcmp(file->lastmodified, buffer)) {
				WebserverFree(file->lastmodified);
				file->lastmodified = (char *)WebserverMalloc(len + 1);
				memcpy(file->lastmodified, buffer, len + 1);
				file->lastmodifiedLength = len;
				return true;
			}
		}

	}

	return true;

}




void	ErrorExit(LPTSTR lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

#ifdef _MSC_VER
 
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
#else
    #warning implementieren
#endif
}



