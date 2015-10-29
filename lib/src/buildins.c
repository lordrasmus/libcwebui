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

#ifdef __GNUC__
	#include "webserver.h"
#endif

extern unsigned char testvar[];






// CGI functions ------------------------------------------------------------------

/*
int buildin_testfkt(socket_info* sock, unsigned char *retbuffer, HttpRequestHeader *pHeader)
{
	WebserverFileInfo *file = create_empty_file(2000);

	file->DataLenght = sprintf( file->Data, "<html><head>HUHU</head><body>na sie??</body></html>" );

	sendHTML(sock,file,retbuffer,pHeader);

	free_file(file);

	return 0;
}

int buildin_basesettings(socket_info* sock, unsigned char *retbuffer, HttpRequestHeader *pHeader)
{
	WebserverFileInfo *file = getFile("empty.html");

	sendHTML(sock,file,retbuffer,pHeader);

	return 0;
}

// Fillins ------------------------------------------------------------------------

int fillin_testfkt( unsigned char *pBuffer, HttpRequestHeader *pHeader)
{
        return sprintf((char*)pBuffer,"Testfunktion : %s",testvar);
}

int fillin_mac( unsigned char *pBuffer, HttpRequestHeader *pHeader )
{
        return sprintf((char*)pBuffer,"%s","00:01:07:17:EE:FF");
}

int fillin_infotext( unsigned char *pBuffer, HttpRequestHeader *pHeader )
{
	return sprintf( (char *) pBuffer, "<H1>Settings have being saved.</H1>" );
}

*/
