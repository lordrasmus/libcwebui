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

#ifdef WEBSERVER_USE_BINARY_FORMAT

	sendHeader ( s,info,info->DataLenght );

	if ( info->RamCached == 1 ) {
		send_pos = 0;
		while ( lenght>FileChunkSize ) {
			ret = WebserverSend ( s->socket,&info->Data[send_pos],FileChunkSize,0 );
			send_pos+=FileChunkSize;
			if ( ret == -1 ) {
				LOG ( ERROR_LOG,"%s","sendFile : Abort Sending Data" );
				break;
			}
			lenght-=FileChunkSize;
		}
		if ( ret != -1 ) {
			ret = WebserverSend ( s->socket,&info->Data[send_pos],lenght,0 );
			if ( ret == -1 ) {
				LOG ( ERROR_LOG,"%s","sendFile : Abort Sending Data" );
			}
		}
	} else {

		unsigned char *buffer=WebserverMalloc ( FileChunkSize,0 );

#ifdef _WEBSERVER_DEBUG_
#if __GNUC__ > 3
		WebServerPrintf ( "Flash Position %u ", ( unsigned int ) info->DataStreamPosition );
#else
		WebServerPrintf ( "Flash Position %u ",info->DataStreamPosition );
#endif

#endif
		//WebserverOpenDataReadStream();
		PlatformOpenDataReadStream ( "data.bin" );
		//WebserverSeekToPosition(info->DataStreamPosition);
		PlatformSeekToPosition ( info->DataStreamPosition );
		ret = 0;
		while ( lenght>FileChunkSize ) {
			PlatformReadBytes ( buffer,FileChunkSize );
			ret = WebserverSend ( s->socket,buffer,FileChunkSize,0 );
			if ( -1 == ret ) {

				//LOG(ERROR_LOG,"sendFile : Abort Sending Data");
				break;
			}
			lenght-=FileChunkSize;
			//WebServerPrintf("Noch %u Bytes\n",lenght);
		}
//      lenght=200
		if ( ret!=-1 ) {
			PlatformReadBytes ( buffer,lenght );
			WebserverSend ( s->socket,buffer,lenght,0 );
		}

		WebserverFree ( buffer );
		PlatformCloseDataStream();
	}

#else
