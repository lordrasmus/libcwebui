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

#include "intern/system_file_access.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif




unsigned char *g_lastmodified;


void WebserverPrintShortInfos(void);

void endHTTPRequest(http_request *s) {
	upload_file_info * f_info;

	clearRenderVariables(s);
	if (s->store != 0) unlockStore(s->store->vars);
	if (s->store_ssl != 0) unlockStore(s->store_ssl->vars);


	ws_list_iterator_start(&s->upload_files);
	while( ( f_info = (upload_file_info*)ws_list_iterator_next(&s->upload_files) ) ){
		WebserverFree(f_info->name);
		WebserverFree(f_info->data);
		WebserverFree(f_info);
	}

	ws_list_iterator_stop(&s->upload_files);
}


void parsePostData(http_request *s){

	char *pos, *pos2;
	char *data = 0;
	char *data_start = 0;
	uint64_t offset = 0;
	uint64_t offset2 = 0;
	uint64_t offset3 = 0;
	char end= 0;
	uint32_t tmp;
	HttpRequestHeader *header;
	uint32_t bound_len;
	char* bound_end;

	if ( s->socket->header->post_buffer == 0 ){
		return;
	}

	if ( s->socket->header->boundary == 0 ){
		return;
	}

	header = WebserverMallocHttpRequestHeader();

	bound_len = WebserverMallocedSize(s->socket->header->boundary) - 1;
	//printf("%d\n",bound_len);

	bound_end = WebserverMalloc( bound_len + 3 );

	strcpy(bound_end, s->socket->header->boundary);
	strcat(bound_end,"--");

	header->method = 1;


	for ( offset = 0; offset < s->socket->header->contentlenght ; offset++ ){

		pos = &s->socket->header->post_buffer[ offset ];

		if ( 0 == strncmp(  pos , bound_end , bound_len + 2) ){
			end = 1;
			break;
		}

		if ( 0 == strncmp(  pos , bound_end , bound_len) ){

			pos = &s->socket->header->post_buffer[ offset + bound_len ];

			offset2 = analyseFormDataLine( s->socket, pos , s->socket->header->contentlenght - offset, header );

			data_start = &pos[offset2];

			for ( offset3 = offset2 ; offset3 < ( s->socket->header->contentlenght - ( offset + bound_len ) ) ; offset3++ ){

				pos2 = &pos[ offset3 ];

				/* Ein Boundary beendet / startet einen Abschnitt */
				if ( 0 == strncmp( pos2 , bound_end , bound_len ) ){
					upload_file_info * file_info;

					/* Am Ender Der Daten ist \r\n darum -2 */
					data = WebserverMalloc( ( offset3 - offset2 ) - 1 );
					memcpy( data, data_start , ( offset3 - offset2 ) - 2);
					data[ ( offset3 - offset2 ) - 2 ] = '\0';

					file_info = WebserverMalloc( sizeof ( upload_file_info ) );

					file_info->data = data;
					file_info->length = ( offset3 - offset2 )  -2;

					tmp = stringfind(header->Content_Disposition, "filename=");
					if ( 0 != tmp){
						uint32_t len;
						tmp += 2; /* = und " stehen noch am anfang */
						len = strlen( header->Content_Disposition ) - tmp - 1; /* am ende ist noch ein " */
						file_info->name= WebserverMalloc( len + 1);
						strncpy( file_info->name , &header->Content_Disposition[ tmp ] , len );
						file_info->name[len] = '\0';
					}else{
						LOG ( CONNECTION_LOG,ERROR_LEVEL,s->socket->socket,"Kein filename in Content_Disposition ( %s )", s->socket->header->Content_Disposition);
						file_info->name= WebserverMalloc( 2 );
						file_info->name[0] = '\0';
					}

					ws_list_append(&s->upload_files, file_info);
					offset = offset3 + offset + bound_len - 1;	/* -1 weil die Hauptschelife +1 macht */
					break;
				}

				/* Ein Boundary mit -- am ende beendet das multipart/form-data */
				if ( 0 == strncmp(  pos2 , bound_end , bound_len + 2) ){
					end = 1;
					break;
				}

			}

		}

		if ( end == 1 ){
			break;
		}

	}

	WebserverFree( s->socket->header->post_buffer );
	s->socket->header->post_buffer = 0;
	s->socket->header->contentlenght = 0;
	s->socket->header->post_buffer_pos = 0;

	WebserverFree( s->socket->header->boundary ) ;
	s->socket->header->boundary = 0;

	WebserverFree( bound_end );

	WebserverFreeHttpRequestHeader( header );


#ifdef ENABLE_DEVEL_WARNINGS
	#warning Funktion noch genauer prüfen
#endif

}


/****************************************************************************
 *																	 	    *
 *	  getHttpRequest(int sock)	:   Hauptfunktion des Servers 			    *
 *																		    *
 *	  laeuft innerhalb eines threads der alle socketverbindung bearbeitet   *
 *    und arbeitet das html protokoll ab              					    *
 *																		    *
 ***************************************************************************/

int getHttpRequest(socket_info* sockp) {
	http_request s;
	WebserverFileInfo *file;

	memset(&s, 0, sizeof(http_request));

	ws_list_init(&s.upload_files);

	initRenderVariable(&s);

	s.socket = sockp;
	s.header = sockp->header;
	sockp->disable_output = 0;

	if (s.header->method == HTTP_UNKNOWN_METHOD) {
		sendMethodNotAllowed(sockp);
		endHTTPRequest(&s);
		return 0;
	}

	if (s.header->error != 0) {
		sendMethodBadRequest(sockp);
		endHTTPRequest(&s);
		return 0;
	}

#ifdef WEBSERVER_USE_SESSIONS
	checkSessionCookie(&s);
#endif

#if _WEBSERVER_CONNECTION_DEBUG_ > 4
	LOG ( CONNECTION_LOG,NOTICE_LEVEL,s.socket->socket,"Request %s",s.header->url );
#endif

#ifdef _WEBSERVER_PARAMETER_DEBUG_
	printHeaderInfo ( s.header );
#endif


	switch ( s.socket->header->method ){
		case HTTP_GET:
			#ifdef _WEBSERVER_DEBUG_
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,s.socket->socket, "GET : %s \t",s.socket->header->url );
			#endif
			break;
		case HTTP_POST:
			#ifdef _WEBSERVER_DEBUG_
			LOG ( CONNECTION_LOG,NOTICE_LEVEL,s.socket->socket, "POST : %s \t",s.socket->header->url );
			#endif
			parsePostData(&s);
			break;
	}


	if ( lockGlobals() == -1 ){
		LOG ( CONNECTION_LOG,ERROR_LEVEL,s.socket->socket, "Fatal Error locking Globals exiting", "" );
		exit ( 1 );
	}

	/*if ( checkCGIFunctions ( &s ) == 0 ) {
	 #ifdef _WEBSERVER_DEBUG_
	 WebServerPrintf ( "  ... OK builtin Function\n" );
	 #endif
	 } else*/
	if (checkBuilinSites(&s) == 0) {
#ifdef _WEBSERVER_DEBUG_
		WebServerPrintf ( "  ... OK builtin Site\n" );
#endif
	} else {
		ws_variable *download;

		if (0 != strlen((char*) s.header->url)){
			file = getFile(s.header->url); /* Eingabe von zB http://192.168.2.80/test.html */
		}else{
			file = getFile((char*) "index.html"); /* Eingabe von zB http://192.168.2.80/ */
		}

		/* durch Eingabe von zB http://192.168.2.80/test.dat?download runterladen durch den Browser erzwingen */
		download = getParameter(&s, "download");
		if ( file && download ) {
			file->ForceDownload = 1;
			if ( download->type == VAR_TYPE_STRING ){
				strncpy((char*)file->ForceDownloadName,download->val.value_string,FORCE_DOWNLOAD_NAME_LENGTH-1);
			}else{
				strcpy((char*)file->ForceDownloadName,"");
			}
		}

#ifdef _WEBSERVER_DEBUG_
#if __GNUC__ > 3
		WebServerPrintf ( " ..%X..  ", ( unsigned int ) ( unsigned long ) file ); // file ist ein Pointer
#else
		WebServerPrintf ( " ..%X..  ",file ); /* file ist ein Pointer */
#endif
#endif
		if (file != 0) {

			if ( file->TemplateFile == 1){
				sendHTMLFile(&s, file); /* Schickt die HTML Seite durch die Template Engine */
			}else{
				sendFile(&s, file); 	/* Sendet die Daten direkt an den Socket ohne den Pagebuffer zu benutzen */
			}

#ifdef _WEBSERVER_DEBUG_
			WebServerPrintf ( " ... OK\n" );
#endif
		} else {
			sendFileNotFound(&s); /* nichts gefunden Fehlermeldung senden */
#ifdef _WEBSERVER_DEBUG_
					WebServerPrintf ( " ... file not found !!\n" );
#endif
		}
	}


	endHTTPRequest(&s);

#ifdef WEBSERVER_USE_WNFS

	if ( file->FileType == FS_WNFS ){
		wnfs_free_file( file );
	}

#endif


	if ( unlockGlobals() == -1 ){
		LOG ( CONNECTION_LOG,ERROR_LEVEL,s.socket->socket, "Fatal Error locking Globals exiting", "" );
		exit ( 1 );
	}

	return 0;
}

void *WebserverHTMLChunkFree(const void *restrict free_element) {
	WebserverFreeHtml_chunk((html_chunk*) free_element);
	return 0;
}


int sendHTMLFile(http_request* s, WebserverFileInfo *file) {

	if ( 0 == prepare_file_content( file ) ){
		WebServerPrintf("File : %s error loading content \n",file->FilePath);
		return -1;
	}

	if ( file->TemplateFile == 0){
		printf("Warning Engine Template Header <%s> not found: %s \n",template_v1_header,file->FilePath);
	}

	processHTML(s, (const char *) file->FilePrefix, file->Url, (const char *) file->Data, file->DataLenght);

	release_file_content( file );

	if (0 > sendHeader(s, file, getChunkListSize(&s->socket->html_chunk_list))) {
		ws_list_attributes_freer(&s->socket->html_chunk_list, WebserverHTMLChunkFree);
		ws_list_clear(&s->socket->html_chunk_list);
		ws_list_attributes_freer(&s->socket->html_chunk_list, 0);
		return -1;
	}
	return 0;
}


int checkBuilinSites(http_request *s) {
	/*if ( !strcmp ( ( char* ) s->header->url,"dataupdate.html" ) ) {
	 unsigned char *chunk_buffer = ( unsigned char * ) WebserverMalloc ( ENGINE_BUFFER_SIZE,0 );
	 int lenght = snprintf ( ( char* ) chunk_buffer,ENGINE_BUFFER_SIZE,"<html><head><Title>Dataupload</title></head><body><form action='dataupdate.func' method='post' enctype='multipart/form-data'><input name='Datei' type='file' maxlength='100000' <input type=submit></form></body></html>" );
	 WebserverFileInfo info;
	 info.FileType=2;
	 if ( 0 < sendHeader ( s,&info,lenght ) ) {
	 //WebserverSend ( s->socket,chunk_buffer,lenght,0 );
	 WebserverSend ( s->socket,chunk_buffer,lenght,0,0 );
	 }
	 WebserverFree ( chunk_buffer );
	 return 0;
	 }*/

	return -1;
}

void WebserverPrintInfos(void) {
	/*for(i=0;i<g_files.FileCount;i++){
	 if(g_files.files[i]->DataLenght == 0) continue;
	 WebServerPrintf("Name : %s\n",g_files.files[i]->Name);

	 WebServerPrintf("Type : ");
	 switch(g_files.files[i]->FileType){
	 case FILE_TYPE_PLAIN:	WebServerPrintf("text/plain\n");break;
	 case FILE_TYPE_HTML:	WebServerPrintf("text/html");	break;
	 case FILE_TYPE_HTML_INC:WebServerPrintf("text/html");	break;
	 case FILE_TYPE_CSS:		WebServerPrintf("image/css");	break;
	 case FILE_TYPE_XML:		WebServerPrintf("text/xml");	break;
	 case FILE_TYPE_JS:		WebServerPrintf("image/js");	break;
	 case FILE_TYPE_JPG:		WebServerPrintf("image/jpeg");	break;
	 case FILE_TYPE_PNG:		WebServerPrintf("image/png");	break;
	 case FILE_TYPE_GIF:		WebServerPrintf("image/gif");	break;
	 case FILE_TYPE_ICO:		WebServerPrintf("image/gif");	break;
	 }

	 #if __GNUC__ > 3
	 WebServerPrintf("   \tLenght : %u",(unsigned int)g_files.files[i]->DataLenght);
	 #else
	 WebServerPrintf("   \tLenght : %d",g_files.files[i]->DataLenght);
	 #endif

	 WebServerPrintf("\n\n");

	 if(g_files.files[i]->FileType<3)
	 size+=g_files.files[i]->DataLenght+1;
	 size+=g_files.files[i]->NameLengt+1;
	 size+=sizeof(WebserverFileInfo);
	 }
	 WebServerPrintf("FileCount : %d\n\n",g_files.FileCount);
	 WebServerPrintf("MemorySize : %d \n",size);
	 WebServerPrintf("-------------------\n\n");
	 */
}

void WebserverPrintShortInfos(void) {
	/*for(i=0;i<g_files.FileCount;i++){
	 if(strlen((char*)g_files.files[i]->Name) < 50)
	 WebServerPrintf("Name : %s",g_files.files[i]->Name);
	 else
	 {
	 WebServerPrintf("Daten sind kaputt!!\n");
	 continue;
	 }
	 switch(g_files.files[i]->FileType){
	 case 1:	WebServerPrintf("\ttext/plain ");break;
	 case 2:	//WebServerPrintf("\tTitle : %s\t text/html ",g_files.files[i]->Title);
	 //size+=g_files.files[i]->TitleLengt+1;
	 WebServerPrintf("\ttext/html ");	break;
	 case 3:	WebServerPrintf("\timage/jpeg ");break;
	 case 4:	WebServerPrintf("\timage/png ");break;
	 case 5:	WebServerPrintf("\timage/gif ");break;
	 }
	 #if __GNUC__ > 3
	 WebServerPrintf("Lenght : %u\n",(unsigned int)g_files.files[i]->DataLenght);
	 #else
	 WebServerPrintf("Lenght : %d\n",g_files.files[i]->DataLenght);
	 #endif
	 WebServerPrintf("\n");

	 size+=g_files.files[i]->DataLenght+1;
	 size+=g_files.files[i]->NameLengt+1;
	 size+=sizeof(WebserverFileInfo);
	 }
	 WebServerPrintf("-------------------\n");
	 */
}

