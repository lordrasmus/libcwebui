



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
	if ( 4 != PlatformSendSocket( wnfs_socket, &len, 4,0 ) )
		goto  error_out;

	if ( len != PlatformSendSocket( wnfs_socket, buffer, len , 0 ))
		goto  error_out;

	if ( 4 != PlatformRecvSocket( wnfs_socket, &len, 4, 0 ))
		goto  error_out;

	if ( len == 0 ){
		return 0;
	}

	WebserverFileInfo *file = create_empty_file( len );
	uint32_t recv_bytes = 0;
	while(1){
		int ret = recv( wnfs_socket, &file->Data[recv_bytes], len -recv_bytes , 0 );
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
	if ( 4 != PlatformSendSocket( wnfs_socket, &len, 4,0 ) )
		goto  error_out;

	if ( len != PlatformSendSocket( wnfs_socket, buffer, len , 0 ))
		goto  error_out;


	len = file->DataLenght;
	if ( 4 != PlatformSendSocket( wnfs_socket, &len , 4,0 ) )
		goto  error_out;

	int ret = send ( wnfs_socket,file->Data,len,0 );
	if ( len != ret ){
		printf("wnfs_store_file error: %d %d %X %m\n",len,ret,file->Data);
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
