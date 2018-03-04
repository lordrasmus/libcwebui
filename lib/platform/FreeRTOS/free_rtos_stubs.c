
#include "webserver.h"


int printf(const char *format, ...){
		return 0;
}

int sprintf(char *str, const char *format, ...){
	return 0;
}

int snprintf(char *str, size_t size, const char *format, ...){
		return 0;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap){
		return 0;
}

int vsprintf(char *str, const char *format, va_list ap){
		return 0;
}

int vprintf(const char *format, va_list ap){
		return 0;
}

int scanf(const char *format, ...){
		return 0;
}

int sscanf(const char *str, const char *format, ...){
		return 0;
}

char *strstr(const char *haystack, const char *needle){
		return 0;
}

char *strncpy(char *dest, const char *src, size_t n){
		return 0;
}

void perror(const char *s){
}

size_t strlen(const char *s){
	return (size_t)s;
}

int strcmp(const char *s1, const char *s2){
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n){
	return 0;
}

char *strchr(const char *s, int c){
	return (char*)s;
}

int strcasecmp(const char *s1, const char *s2){
	return 0;
}

int atoi(const char *nptr){
		return 0;
}

long atol(const char *nptr){
		return 0;
}

/*void breakEvents(void){
}*/

int puts(const char *s){
	return 0;
}

int fflush( FILE *datenstrom){
		return 0;
}

int ioctl(int fd, unsigned long request, ...){
		return 0;
}

int close(int fd){
	return 0;
}

char *strncat(char *dest, const char *src, size_t n){
		return 0;
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
		return 0;
}

int shutdown(int sockfd, int how){
		return 0;
}

pid_t getpid(void){
		return 0;
}

// __attribute__ ( )
void exit ( int status ) {
	status = status;
}

int sem_init(sem_t *sem, int pshared, unsigned int value){
	return 0;
}
int sem_wait(sem_t *sem){
	return 0;	
}
int sem_post(sem_t *sem){
		return 0;
}

void __assert_func(void){

}

void __errno( void ){
}

const char * __locale_ctype_ptr( void ){
		return 0;
}

/*void _impure_ptr( void ){}*/

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count){
	return 0;
}

void *malloc(size_t size){
	size = size;
	return 0;
}
void free(void *ptr){
	ptr = ptr;
}

void *realloc(void *ptr, size_t size){
	size = size;
	return ptr;
}


int memcmp(const void *s1, const void *s2, size_t n){
	s2=s1;
	s1=s2;
	n=n;
	return 0;
}

void *memmove(void *dest, const void *src, size_t n){
	n=n;
	dest = dest;
	src = src;
	return 0;
}


#include <reent.h>

/* Note that there is a copy of this in sys/reent.h.  */
#ifndef _ATTRIBUTE_IMPURE_PTR_
#define _ATTRIBUTE_IMPURE_PTR_
#endif

#ifndef _ATTRIBUTE_IMPURE_DATA_
#define _ATTRIBUTE_IMPURE_DATA_
#endif

//static struct reent __ATTRIBUTE_IMPURE_DATA_ impure_data = _REENT_INIT(impure_data);
struct _reent *__ATTRIBUTE_IMPURE_PTR__ _impure_ptr = 0;// &impure_data;

