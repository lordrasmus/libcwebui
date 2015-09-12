


#include <stdio.h>
#include <inttypes.h>


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


enum
{
  TINFL_FLAG_PARSE_ZLIB_HEADER = 1,
  TINFL_FLAG_HAS_MORE_INPUT = 2,
  TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,
  TINFL_FLAG_COMPUTE_ADLER32 = 8
};
size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

#include <zlib.h>


static const unsigned char* read_file( const unsigned char *alias, const unsigned char* data, uint64_t* offset_p, uint32_t* compressed_p ){

	const unsigned char *name;
	const unsigned char *etag;
	const unsigned char *ret;
	uint32_t size = 0;
	uint32_t real_size,compresed_size;
	
	
	
	WebserverFileInfo *file = 0;
	
	file = (WebserverFileInfo*) WebserverMalloc ( sizeof ( WebserverFileInfo ) );
	memset(file, 0, sizeof(WebserverFileInfo));
	
	*compressed_p = read_uint32( data, offset_p );
	
	name = read_string( data, offset_p, &size );
	
	etag = read_string( data, offset_p, &size );
	
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
	
	if ( *compressed_p == 1 ){
		real_size = read_uint32( data, offset_p );
		compresed_size = read_uint32( data, offset_p );
		
		ret = &data[*offset_p];
		
		*offset_p += compresed_size;
		
		file->DataLenght = compresed_size;
		
		
		//printf("decomp_size : %d\n",decomp_size); 
		
		//printf("   Name : %s ( real %" PRIu32 " / comp %" PRIu32 " ) \n",name,real_size,compresed_size);
		
		//size_t s = tinfl_decompress_mem_to_mem( a, decomp_size, ret, compresed_size, TINFL_FLAG_PARSE_ZLIB_HEADER );
		//printf("%d %s\n",s,a);
		
		if ( isTemplateFile( file->Url ) ){
			
			unsigned long decomp_size = real_size + 32;
			
			unsigned char* decomp_buffer = malloc( decomp_size );
			
			file->TemplateFile = 1;
		
			printf("decompressing template: %s\n",name);
			z_stream strm;  
			strm.next_in = (unsigned char*)ret;  
			strm.avail_in = compresed_size; 
			 
			strm.next_out = decomp_buffer;
			strm.avail_out = real_size;
			strm.total_out = 0;  
			
			strm.zalloc = Z_NULL;  
			strm.zfree = Z_NULL; 
			
			if (inflateInit2(&strm, (16+MAX_WBITS)) != Z_OK) {  
				printf("e1\n");
			}
			
			int err = inflate (&strm, Z_SYNC_FLUSH);  
			switch(err){
				case Z_STREAM_END: 
					//printf("Z_STREAM_END\n"); 
					break;
					
				default: printf("unknown2 %d \n",err);
			}
			
			ret = decomp_buffer;
			file->DataLenght = real_size;
			*compressed_p = 0;
		}
		//printf("zlib2 : %s\n",a);
		
	}else{
		
		if ( isTemplateFile( file->Url ) ){
			file->TemplateFile = 1;
		}
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
