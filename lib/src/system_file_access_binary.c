

#ifdef WEBSERVER_USE_BINARY_FORMAT
	int i; //,i2,i3;
//int sig[4];
//unsigned long flashposition=0;
	WebserverFileInfo *info;
	unsigned int DataLenght;
	unsigned long to_read,read_pos;

	WDT_RESET

	if ( PlatformOpenDataReadStream ( "data.bin" ) ==false ) {
		LOG (FILESYSTEM_LOG,ERROR_LEVEL, "Webserver Load Data failed\r\n" );
		return false;
	}
	//WebserverPrintPosition();

	WDT_RESET

	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Webserver Load Data\r\n" );

	DataLenght = readInt();
	g_lastmodified=WebserverMalloc ( DataLenght + 1 );
	PlatformReadBytes ( g_lastmodified,DataLenght );
	g_lastmodified[DataLenght]='\0';

#ifdef _WEBSERVER_DEBUG_
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Last Modified Lenght:  %u\n",DataLenght );
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Last Modified :  %s\n",g_lastmodified );
#endif

	g_files.FileCount = readInt();
	g_files.files= ( WebserverFileInfo** ) WebserverMalloc ( sizeof ( WebserverFileInfo* ) * g_files.FileCount );

#ifdef _WEBSERVER_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "Files : %d\n",g_files.FileCount );
#endif

	for ( i=0;i<g_files.FileCount;i++ ) {
		WDT_RESET

		info = ( WebserverFileInfo* ) WebserverMalloc ( sizeof ( WebserverFileInfo ) );
		memset ( info,0,sizeof ( WebserverFileInfo ) );
		if ( info==0 ) {
			LOG ( FILESYSTEM_LOG,ERROR_LEVEL,"WebServerloadData Malloc FileInfo Struct : Kein Speicher verfuegbar\n" );
			return false;
		}

		info->NameLengt = readInt();
		info->Name = ( unsigned char* ) WebserverMalloc ( info->NameLengt + 1 );
		if ( info->Name==0 ) {
			LOG ( FILESYSTEM_LOG,ERROR_LEVEL,"WebServerloadData Malloc Name : Kein Speicher verfuegbar (%d)\n",info->NameLengt );
			return false;
		}
		PlatformReadBytes ( info->Name,info->NameLengt );
		info->Name[info->NameLengt]='\0';

		PlatformReadBytes ( & ( info->FileType ),1 );
		PlatformReadBytes ( & ( info->Id ),1 );

		setFileType ( info );
		info->DataLenght = readInt();
//if ((info->FileType == FILE_TYPE_HTML)||(info->FileType == FILE_TYPE_HTML_INC)) 			// Daten in den Speicher laden ( HTML , Text)
		if ( info->FileType <= WEBSERVER_MAX_FILEID_TO_RAM ) {
			info->Data = ( unsigned char* ) WebserverMalloc ( sizeof ( unsigned char ) *info->DataLenght );
			if ( info->Data==0 ) {
				LOG (FILESYSTEM_LOG,ERROR_LEVEL, "WebServerloadData : Kein Speicher verfuegbar\n" );
				return false;
			}
			to_read = info->DataLenght;
			read_pos = 0;
			while ( to_read >= 1000 ) {
				PlatformReadBytes ( &info->Data[read_pos],1000 );
				to_read -= 1000;
				read_pos += 1000;
			}
			PlatformReadBytes ( &info->Data[read_pos],to_read );

			info->RamCached = 1;
		} else { // Position der Daten im Datenstream merken
			info->DataStreamPosition=PlatformGetDataStreamPosition();
			PlatformSeek ( info->DataLenght );
			//WebserverSeekToPosition(info->DataLenght);
		}

#ifdef _WEBSERVER_DEBUG_
		LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "Name : %s (%d) ID : %d\n",info->Name,info->NameLengt,info->Id );
		LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Datalenght %lu\n\n",info->DataLenght );
//LOG("File : %s ",info->Name);
//WebserverPrintPosition();
#endif

//      WebserverSeekToPosition(
//		WebserverReadBytes(debug_buffer,info->DataLenght);

		g_files.files[i]=info;

	}

	PlatformCloseDataStream();

	return true;
#endif
}

char WebServerReadDataStart(void) {
	unsigned int data[5];
	data[0] = readInt(); /* signatur */
	data[1] = readInt();
	data[2] = readInt();
	data[3] = readInt();

	data[4] = readInt(); /* Laenge des Datensatzes */

	if ((data[0] != 1) || (data[1] != 255) || (data[2] != 1)
			|| (data[3] != 255)) {
		LOG(FILESYSTEM_LOG, ERROR_LEVEL, 0,
				"Webserver Datensignatur nicht gefunden (0x%X,0x%X,0x%X,0x%X) !!!",
				data[0], data[1], data[2], data[3]);
		return false;
	}

	return true;
}


#ifdef jio

void WebserverSaveDataChunk ( unsigned char *data,unsigned int lenght ) {
	PlatformWriteBytes ( data,lenght );
}

void WebserverSavaDataFinish ( void ) {
	PlatformCloseDataStream();
}

void WebServerSaveDataStart ( unsigned int lenght ) {
	LOG ( FILESYSTEM_LOG,NOTICE_LEVEL,"Saving Data ( %u bytes ) ...\n",lenght );
	writeInt ( 1 );
	writeInt ( 255 );
	writeInt ( 1 );
	writeInt ( 255 );
	writeInt ( lenght );
}

#endif

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
