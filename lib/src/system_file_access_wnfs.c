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



#ifdef __GNUC__
#include "webserver.h"
#endif


#ifdef WEBSERVER_USE_WNFS

#include "intern/system_file_access.h"



WebserverFileInfo* wnfs_get_file( unsigned char* name){

	if ( wnfs_socket == 0 ){
		return 0;
	}


	char buffer[100];
	//printf("wnfs_get_file  : %s\n", name );
	uint32_t len = sprintf(buffer,"{\"op\":\"get_file\",\"file\":\"%s\"}",name);
	int ret;
	if ( 4 != PlatformSendSocket( wnfs_socket, (unsigned char *)&len, 4,0 ) )
		goto  error_out;

	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)buffer, len , 0 );
	if ( ( ret <= 0 ) || ( len != (uint32_t)ret) )
		goto  error_out;
		
	
	ret = PlatformRecvSocket( wnfs_socket, (unsigned char *)&len, 4, 0 );
	if ( ret < 0 )
		goto  error_out;

	if ( len == 0 ){
		return 0;
	}

	WebserverFileInfo *file = create_empty_file( len );
	uint32_t recv_bytes = 0;
	while(1){
		ret = recv( wnfs_socket, (char*)&file->Data[recv_bytes], len -recv_bytes , 0 );
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
	file->FilePrefix = (unsigned char*)"/";
	setFileType(file);

	file->TemplateFile = 1;
	#warning Das hier besser lösen
	
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
	if ( 4 != ret )
		goto  error_out;

	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)buffer, len , 0 );
	if ( ( ret <= 0 ) || ( len != (uint32_t)ret) )
		goto  error_out;

	len = file->DataLenght;
	
	ret = PlatformSendSocket( wnfs_socket, (const unsigned char *)&len , 4,0 );
	if ( 4 != ret )
		goto  error_out;

	ret = send ( wnfs_socket,file->Data,len,0 );
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
	return ;

}



void wnfs_free_file( WebserverFileInfo* file ){

	free_empty_file( file );

}


#endif
