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


#include <stdio.h>
#include <inttypes.h>

#include <zlib.h>

#include "webserver.h"

#include "intern/system_file_access.h"

#ifdef WEBSERVER_USE_BINARY_FORMAT

static uint64_t read_uint64( const unsigned char* data, uint64_t* offset_p ){

	uint64_t val;

	int offset = *offset_p;

	val  = ((uint64_t)data[offset++])<<0;
	val += ((uint64_t)data[offset++])<<8;
	val += ((uint64_t)data[offset++])<<16;
	val += ((uint64_t)data[offset++])<<24;
	val += ((uint64_t)data[offset++])<<32;
	val += ((uint64_t)data[offset++])<<40;
	val += ((uint64_t)data[offset++])<<48;
	val += ((uint64_t)data[offset++])<<56;
	*offset_p = offset;

	return val;

}

static uint32_t read_uint32( const unsigned char* data, uint64_t* offset_p ){

	uint32_t val;

	int offset = *offset_p;

	val  = ((uint32_t)data[offset++])<<0;
	val += ((uint32_t)data[offset++])<<8;
	val += ((uint32_t)data[offset++])<<16;
	val += ((uint32_t)data[offset++])<<24;

	*offset_p = offset;

	return val;
}

static const unsigned char* read_string( const unsigned char* data, uint64_t* offset_p, uint32_t* length){

	const unsigned char* ret;

	*length = read_uint32( data, offset_p );

	ret = &data[*offset_p];

	*offset_p += *length + 1;

	return ret;
}



/*static void print_next_bytes( unsigned char* data, uint64_t offset){

	for ( uint64_t i = offset ; i < offset + 20 ; i++ )
		printf("%d ",data[i]);
	printf("\n\n");
}*/




static const unsigned char* read_file( const unsigned char *alias, const unsigned char* data, uint64_t* offset_p, uint32_t* compressed_p ){

	const unsigned char *name;
	const unsigned char *etag;
	const unsigned char *ret;
	uint32_t size = 0;
	uint32_t real_size,compresed_size;
	uint32_t template;



	WebserverFileInfo *file = 0;

	file = (WebserverFileInfo*) WebserverMalloc ( sizeof ( WebserverFileInfo ) );
	memset(file, 0, sizeof(WebserverFileInfo));

	*compressed_p = read_uint32( data, offset_p );

	name = read_string( data, offset_p, &size );

	etag = read_string( data, offset_p, &size );

	template = read_uint32( data, offset_p );

	// TODO lastmodified noch einbauen
	//file->lastmodified = etag;
	//file->lastmodifiedLength = strlen ( etag );

#ifndef WEBSERVER_DISABLE_CACHE
	file->etag = etag;
	file->etagLength = strlen ( (char*)etag );
#endif

	file->fs_type = FS_BINARY;
	file->RamCached = 1;
	file->FilePrefix = alias;

	copyFilePath(file, name);
	copyURL(file, name);
	setFileType(file);

	if ( *compressed_p > 0 ){

		real_size = read_uint32( data, offset_p );
		compresed_size = read_uint32( data, offset_p );

		ret = &data[*offset_p];

		file->CompressedData = ret;
		file->CompressedDataLenght = compresed_size;
		file->RealDataLenght = real_size;

		*offset_p += compresed_size;

		file->DataLenght = compresed_size;

		//printf("   Name : %s ( real %" PRIu32 " / comp %" PRIu32 " ) \n",name,real_size,compresed_size);


		if ( template ){

			unsigned long decomp_size = real_size + 32;

			unsigned char* decomp_buffer = WebserverMalloc( decomp_size );

			file->TemplateFile = 1;

			size_t new_size = 0;

			switch ( *compressed_p ){

				case 2: 	// deflate compression

					printf("decompressing template ( deflate ) : %s\n",name);

					new_size = tinfl_decompress_mem_to_mem( decomp_buffer, decomp_size, ret, compresed_size, 0 );
					//printf("%d\n%s\n",new_size,decomp_buffer);
					break;

			//size_t s = uncompress(decomp_buffer, decomp_size, ret, compresed_size);
			//printf("%d %s\n",s,decomp_buffer);

				case 1:{		// gzip compression

						printf("decompressing template ( gzip ) : %s\n",name);

						z_stream strm;
						strm.next_in = (unsigned char*)ret;
						strm.avail_in = compresed_size;

						strm.next_out = decomp_buffer;
						strm.avail_out = real_size;
						strm.total_out = 0;

						strm.zalloc = Z_NULL;
						strm.zfree = Z_NULL;

						if (inflateInit2(&strm, (16+MAX_WBITS) ) != Z_OK) {
							printf("inflateInit2 Error\n");
							exit(1);
						}

						int err = inflate (&strm, Z_SYNC_FLUSH);
						switch(err){
							case Z_OK: 	break;
							case Z_STREAM_END: 	break;

							case Z_NEED_DICT:      printf("Z_NEED_DICT\n"); exit( 1 );
							case Z_STREAM_ERROR:   printf("Z_STREAM_ERROR\n"); exit( 1 );
							case Z_DATA_ERROR:     printf("Z_DATA_ERROR\n"); exit( 1 );
							case Z_MEM_ERROR:      printf("Z_MEM_ERROR\n"); exit( 1 );
							case Z_BUF_ERROR:      printf("Z_BUF_ERROR\n"); exit( 1 );
							case Z_VERSION_ERROR:  printf("Z_VERSION_ERROR\n"); exit( 1 );

							case Z_ERRNO:          printf("Z_ERRNO: %m\n"); exit( 1 );



							default:
								printf("unknownerror %d \n",err);
								exit(1);
						}

						new_size = strm.total_out;
					}
					break;
			}

			if ( new_size != real_size ){
				printf("Decomp Size mismatch %zu != %d\n",new_size, real_size );
				exit(1);
			}

			ret = decomp_buffer;
			file->DataLenght = real_size;
			*compressed_p = 0;
		}
		//printf("zlib2 : %s\n",a);

	}else{

		real_size = read_uint32( data, offset_p );

		//printf("   Name : %s ( %" PRIu32 " ) \n",name,real_size);

		ret = &data[*offset_p];

		file->DataLenght = real_size;

		*offset_p += real_size + 1;
	}



	file->Compressed = *compressed_p;
	file->Data = ret;

#ifdef _WEBSERVER_FILESYSTEM_CACHE_DEBUG_
	LOG (FILESYSTEM_LOG,NOTICE_LEVEL,0, "Add File Info Node","" );
#endif

	addFileToCache(file);

	return ret;

}


void read_binary_data( const unsigned char* data ){
	uint64_t offset = 0;
	uint32_t size = 0;
	const unsigned char *alias;
	uint64_t time;
	uint32_t files;
	uint32_t compressed;


	alias = read_string( data, &offset, &size );
	time = read_uint64( data, &offset );
	files = read_uint32( data, &offset );

	printf("Alias : %s\n",alias);
	printf("Time  : %" PRIu64 "\n",time);
	printf("Files : %" PRIu32 "\n",files);

	for ( uint32_t i = 0 ; i < files ; i++ ){
		read_file( alias, data , &offset , &compressed);

	}

}

#endif
