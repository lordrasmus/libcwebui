
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <linked_list.h>

typedef struct{
	char name[100];
	char value[100];
}list_test_ele;

void print_ele( list_test_ele* ele ){
	if ( ele == 0 ){
		printf("ele == 0\n");
		return;
	}
		
	printf("name : %s -> %s\n",ele->name,ele->value);
}

void dumplist( list_t* l ){
	printf("   -- dump  -- \n");
	printf("Elenemts : %d \n",ws_list_size( l ));
	ws_list_iterator_start( l );
	while( ws_list_iterator_hasnext( l ) ){
		list_test_ele* ele = ws_list_iterator_next( l );
		printf("   ");
		print_ele( ele );
	}
	ws_list_iterator_stop( l );
}

list_test_ele* gen_ele( char* name, char* value){
	list_test_ele* ret = malloc( sizeof( list_test_ele ) );
	sprintf( ret->name , "%s", name );
	sprintf( ret->value , "%s", value );
	return ret;
}

int seeker(const void *el, const void *indicator){
	list_test_ele *ele = (list_test_ele*)el;
	
	if ( 0 == strcmp( ele->name, (const char*)indicator ) ){
		return 1;
	}
	
	return 0;
}

void *ele_freer(const void * free_element){
	list_test_ele *ele = (list_test_ele*)free_element;
	
	//printf("free %p  ",ele);
	printf("free ");
	print_ele( ele );
	
	free( (void*)free_element );
}

int main( int argc, char** argv){
	
	
	list_t test_list;
	list_test_ele *ele,*ele2;
	
	list_test_ele *e0,*e1,*e2,*e3;
	
	e0 = gen_ele( "ele0",  "value0" );
	e1 = gen_ele( "ele1",  "value1" );
	e2 = gen_ele( "ele2",  "value2" );
	e3 = gen_ele( "ele3",  "value3" );
	
	ws_list_init( &test_list );
	
	dumplist( &test_list );
	
	ws_list_attributes_freer( &test_list, ele_freer );
	ws_list_attributes_seeker( &test_list, seeker );
	
	ws_list_append( &test_list, e0, 0 );
	ws_list_append( &test_list, e1, 0 );
	ws_list_append( &test_list, e2, 0 );
	ws_list_append( &test_list, e3, 0 );
	
	printf("------------ ws_list_get_at ----------------\n");

	ele2 = ws_list_get_at( &test_list, 2 );	
	print_ele( ele2 );
	
	dumplist( &test_list );
	
	printf("------------ ws_list_extract_at ----------------\n");
	
	ele2 = ws_list_extract_at( &test_list, 1 );
	print_ele( ele2 );
	ele_freer( ele2 );
	
	dumplist( &test_list );
	
	printf("------------ ws_list_seek ----------------\n");
	
	ele2 = ws_list_seek( &test_list , "ele0" );
	print_ele( ele2 );
	
	printf("------------ ws_list_empty 1 ----------------\n");
	
	if ( ws_list_empty( &test_list ) == 0) 
		printf("Liste gefüllt\n");
	else
		printf("Liste leer\n");
	
	printf("------------ ws_list_delete 2 ----------------\n");
	
	ws_list_delete( &test_list, e2 );
	ele_freer( e2 );
	dumplist( &test_list );
	
	printf("------------ ws_list_destroy ----------------\n");
	ws_list_destroy( &test_list );
	
	printf("------------ ws_list_empty 2 ----------------\n");
	
	if ( ws_list_empty( &test_list ) == 0 ) 
		printf("Liste gefüllt\n");
	else
		printf("Liste leer\n");
		
	
	e0 = gen_ele( "ele0",  "value0" );
	e1 = gen_ele( "ele1",  "value1" );
	e2 = gen_ele( "ele2",  "value2" );
	
	ws_list_append( &test_list, e0, 0 );
	ws_list_append( &test_list, e1, WS_LIST_APPEND_FIRST );
	ws_list_append( &test_list, e2, WS_LIST_APPEND_FIRST );
	
	dumplist( &test_list );
	
	ws_list_empty( &test_list );
}
