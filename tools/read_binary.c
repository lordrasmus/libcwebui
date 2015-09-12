

extern char data__[];

#include <stdio.h>
#include <inttypes.h>


uint64_t read_uint64( unsigned char* data, uint64_t* offset_p ){
	
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

uint32_t read_uint32( unsigned char* data, uint64_t* offset_p ){
	
	uint32_t val;
	
	int offset = *offset_p;
	
	val  = ((uint32_t)data[offset++])<<0;
	val += ((uint32_t)data[offset++])<<8;
	val += ((uint32_t)data[offset++])<<16;
	val += ((uint32_t)data[offset++])<<24;
	
	*offset_p = offset;
	
	return val;
	
}

char* read_string( unsigned char* data, uint64_t* offset_p, uint32_t* length){
	
	char* ret;
		
	*length = read_uint32( data, offset_p );
	 
	ret = &data[*offset_p];
	
	*offset_p += *length + 1;
	
	return ret;
}



void print_next_bytes( unsigned char* data, uint64_t offset){

	for ( int i = offset ; i < offset + 20 ; i++ )
		printf("%d ",data[i]); 
	printf("\n\n");
}


unsigned char* read_file( unsigned char* data, uint64_t* offset_p, uint32_t* compressed_p ){

	char *name;
	unsigned char *ret;
	uint32_t size = 0;
	uint32_t real_size,compresed_size;
	
	*compressed_p = read_uint32( data__, offset_p );
	
	name = read_string( data__, offset_p, &size );
	
	if ( *compressed_p == 1 ){
		real_size = read_uint32( data__, offset_p );
		compresed_size = read_uint32( data__, offset_p );
		
		*offset_p += compresed_size;
		
		printf("   Name : %s ( real %" PRIu32 " / comp %" PRIu32 " ) \n",name,real_size,compresed_size);
	}else{
		real_size = read_uint32( data__, offset_p );
				
		printf("   Name : %s ( %" PRIu32 " ) \n",name,real_size);
		
		ret = &data[*offset_p];
		
		*offset_p += real_size + 1;
	}
	
	return ret;
	
}

int main() {

	printf("\n\nHallo Binary \n\n");
	
	uint64_t offset = 0;
	uint32_t size = 0;
	char *alias;
	unsigned char *data;
	uint64_t time;
	uint32_t files;
	uint32_t compressed;
	
	
	alias = read_string( data__, &offset, &size );
	time = read_uint64( data__, &offset );
	files = read_uint32( data__, &offset );
	
	printf("Alias : %s\n",alias);
	printf("Time  : %" PRIu64 "\n",time);
	printf("Files : %" PRIu32 "\n",files);
	
	for ( int i = 0 ; i < files ; i++ ){
		data = read_file( data__ , &offset , &compressed);
		
		if ( compressed == 0 ){
			//printf("%s\n",data);
		}else{
			//printf("compressed\n");
		}
	}
	
	
	printf("\n");
	
	
	print_next_bytes( data__, offset );
	

	return 0;
}
