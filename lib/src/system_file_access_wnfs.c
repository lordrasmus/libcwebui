/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/



#include "webserver.h"


#ifdef WEBSERVER_USE_WNFS

#include "intern/system_file_access.h"



WebserverFileInfo* wnfs_get_file( const char* name ){

	if ( wnfs_socket == 0 ){
		return 0;
	}


	char buffer[100];
	//printf("wnfs_get_file  : %s\n", name );
	uint32_t len = sprintf(buffer,"{\"op\":\"get_file\",\"file\":\"%s\"}",name);
	int ret;
	if ( 4 != PlatformSendSocket( wnfs_socket, (unsigned char *)&len, 4,0 ) ){
		goto  error_out;
	}

	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)buffer, len , 0 );
	if ( ( ret <= 0 ) || ( len != (uint32_t)ret) ){
		goto  error_out;
	}
		
	
	ret = PlatformRecvSocket( wnfs_socket, (unsigned char *)&len, 4, 0 );
	if ( ret < 0 ){
		goto  error_out;
	}

	if ( len == 0 ){
		return 0;
	}

	#define MAX_FILE_SIZE 1 * 1024 * 1024 * 1024 // 1 GB
	// Validierung einfügen!
	if ( len > MAX_FILE_SIZE ){
		printf("Invalid length received: %u > MAX_FILE_SIZE ( %d )\n", len, MAX_FILE_SIZE);
		goto error_out;
	}

	WebserverFileInfo *file = create_empty_file( len );
	uint32_t recv_bytes = 0;
	while(1){
		ret = PlatformRecvSocket( wnfs_socket, (unsigned char *)&file->Data[recv_bytes], len -recv_bytes , 0 );
		if ( ret <= 0 ){
			goto  error_out2;
		}

		recv_bytes += ret;
		if ( len > recv_bytes ){
			//printf("l1: %d l2: %d\n",len,ret);
			continue;
		}
		break;
	}

	file->fs_type = FS_WNFS;

	copyFilePath(file, name);
	copyURL(file, name);

	/* tmp_var ist permanent in der liste der prefixe darum pointer direkt nehmen */
	file->FilePrefix = "/";
	setFileType(file);

	file->TemplateFile = 1;
#ifdef ENABLE_DEVEL_WARNINGS	
	#warning Das hier besser lösen
#endif
	
	if ( file->DataLenght > (int)sizeof( template_v1_header ) ) {

		if ( 0 == memcmp( template_v1_header, file->Data, sizeof( template_v1_header ) -1 ) ){
			//printf("getFileInformation: Engine Template V1 Header found : %s  \n", name_tmp);
			file->TemplateFile = 1;
		}
	}

	return file;

error_out2:

	free_empty_file( file );

error_out:
	printf("wnfs_get_file error: %m\n");
	PlatformCloseSocket( wnfs_socket );
	wnfs_socket = 0;
	return 0;
}

void wnfs_store_file( WebserverFileInfo* file ){

	if ( wnfs_socket == 0 ){
		return;
	}

	//printf("wnfs_store_file: %s\n",file->Url);

	char buffer[1000];

	uint32_t len = sprintf(buffer,"{\"op\":\"put_file\",\"file\":\"%s\"}",file->Url);
	int ret;
	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)&len, 4,0 );
	if ( 4 != ret ){
		goto  error_out;
	}

	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)buffer, len , 0 );
	if ( ( ret <= 0 ) || ( len != (uint32_t)ret) ){
		goto  error_out;
	}

	len = file->DataLenght;
	
	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)&len , 4,0 );
	if ( 4 != ret ){
		goto  error_out;
	}

	ret = PlatformSendSocket ( wnfs_socket,file->Data,len,0 );
	if ( ( ret <= 0 ) || ( len != (uint32_t)ret) ){
		printf("wnfs_store_file error: %d %d %X %m\n",len,ret,(unsigned int)(*file->Data));
		goto  error_out;
	}

	//if ( len != PlatformSendSocket( wnfs_socket, file->Data , len, 0 ))
	//	goto  error_out;


	return;

error_out:
	printf("wnfs_store_file error: %m\n");
	PlatformCloseSocket( wnfs_socket );
	wnfs_socket = 0;

}



void wnfs_free_file( WebserverFileInfo* file ){

	free_empty_file( file );

}


#endif
