
#include <stdio.h>

#include <webserver.h>




int main( int argc, char** argv){

	char buf[10000];


	memset(buf, 0, 10000);
	int len = read(0, buf, 10000);

	url_decode(buf);
	printf("%s",buf);


	return 0;
}
