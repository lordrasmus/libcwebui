/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#ifdef WebserverUseNFS
extern socket_info NFSSocket;
#endif


#ifdef WebserverUseNFS
//char nfsbuffer[50000];
static socket_info NFSSocket;

bool connectNFSServer ( void ) {
	static char ip[4];

	ip[0]=WEBSERVER_NFS_IP1;
	ip[1]=WEBSERVER_NFS_IP2;
	ip[2]=WEBSERVER_NFS_IP3;
	ip[3]=WEBSERVER_NFS_IP4;

	//LOG("Webserver Connect NFS\n");
	NFSSocket.socket = PlatformCreateSocket();
	return PlatformConnectSocket ( NFSSocket.socket,ip,5555 );

	//status = connect(NFSSocket.socket, (struct sockaddr *)&dstAddr, sizeof(dstAddr));
}
#endif

#ifdef WebserverUseNFS
WebserverFileInfo *getFileNFS ( char *name ) {
	unsigned long recv,le,ret;
	static char buffer[300];
	char cle;

#ifdef _WEBSERVER_FILESYSTEM_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL, "Get File Use NFS : %s\r\n",name );
#endif

	if ( false == connectNFSServer() ) {
		LOG (FILESYSTEM_LOG,ERROR_LEVEL,"NFS Server not avaible" );
		return 0;
	}

	for ( i=0;i<WEBSERVER_RAM_FILE_SPACE;i++ ) {
		if ( 0==strcmp ( ( char* ) g_files.files[i]->Name, ( char* ) name ) ) {
			break;
		}
	}
	if ( i==WEBSERVER_RAM_FILE_SPACE ) {
		for ( i=0;i<WEBSERVER_RAM_FILE_SPACE;i++ ) {
			if ( g_files.files[i]->Used==0 )
			break;
		}
	}

	g_files.files[i]->Id=0;
	g_files.files[i]->Used=1;
	g_files.files[i]->RamCached=1;

	copyFileName ( g_files.files[i],name );
	setFileType ( g_files.files[i] );

	cle= sprintf ( buffer,"getFile:%s\0",name );
	WebserverPlatformSend ( &NFSSocket,&cle,1,0 );
	WebserverPlatformSend ( &NFSSocket,buffer,cle,0 );
	WebserverPlatformRecv ( &NFSSocket,buffer,1,0 );
	//g_files.files[i]->FileType = buffer[0];
	WebserverPlatformRecv ( &NFSSocket,buffer,4,0 );
	le= buffer[0]<<24;
	le+= buffer[1]<<16;
	le+= buffer[2]<<8;
	le+= ( unsigned char ) buffer[3];

	cle=i;
	g_files.files[i]->DataLenght = le;
	if ( g_files.files[i]->Data != 0 )
	WebserverFree ( g_files.files[i]->Data );
	g_files.files[i]->Data = ( unsigned char* ) WebserverMalloc ( sizeof ( unsigned char ) * ( le ) );
	recv=0;
	do {
		ret=WebserverPlatformRecv ( &NFSSocket,&g_files.files[i]->Data[recv],le-recv,0 );
		recv+=ret;
	}while ( recv<le );

	//LOG("Data : %s\r\n",g_files.files[i]->Data);

	WebserverCloseSocket ( &NFSSocket );
	if ( le == 0 )
	return 0;

	return g_files.files[i];
}

#endif
