
#include "webserver.h"



#ifdef WEBSERVER_USE_SESSIONS
static 	unsigned int guid;
void 	PlatformGetGUID ( char* buf,SIZE_TYPE length ) {
	
	#warning nur eine testimplementierung
    SIZE_TYPE l=0;
	int ret;

    if ( buf == 0 ){
    	return;
    }

    ret = snprintf ( buf, length, "\"Test %d", guid++ );
    if ( ret < 0 ){
		buf[0]='\0';
		perror("PlatformGetGUID: snprintf failed\n");
		return;
	}
	l = (SIZE_TYPE)ret;
    for ( ; l < length ; l++ ) {
        buf[l]='+';
    }
    buf[length-2]='\"';
    buf[length-1]='\0';
}
#endif

/********************************************************************
*																	*
*					Thread Lock ( Mutex )							*
*																	*
********************************************************************/


int PlatformCreateMutex(WS_MUTEX* m){

	if ( m == 0 ){
		printf("PlatformLockMutex wurde nicht initialisiert\n");
		return EINVAL;
	}

	m->locked = 0;
	k_mutex_init(&m->handle);
	
	return 0;
}

int PlatformLockMutex(WS_MUTEX* m){
	
	if ( m == 0 ){
		printf("PlatformLockMutex wurde nicht initialisiert\n");
		return EINVAL;
	}
	
	k_mutex_lock(&m->handle, K_FOREVER);
	m->locked++;
	if ( m->locked > 1){
		printf("PlatformLockMutex wurde doppelt gelocked\n");
	}
	return 0;
}

int PlatformUnlockMutex(WS_MUTEX* m){
	/*printf("Unlocking %X\n",m);*/
	if ( m == 0 ){
		printf("PlatformLockMutex wurde nicht initialisiert\n");
		return EINVAL;
	}

	m->locked--;
	k_mutex_unlock( &m->handle );
	return 0;
}

int PlatformDestroyMutex(WS_MUTEX* m){
	/*printf("Destroy Mutex\n");*/
	if ( m == 0 ){
		return EINVAL;
	}
	if ( m->locked != 0){
		printf("PlatformDestroyMutex Mutex ist gelocked\n");
	}
	
	printf("PlatformDestroyMutex not implemented\n");
	
	return 0;
}

/**************************************************************
*                                                             *
*                   Speicher Verwaltung                       *
*                                                             *
**************************************************************/

void*	PlatformMalloc ( SIZE_TYPE size ) {
	return malloc( size );
}


void	PlatformFree ( void *mem ) {
	free( mem );
}


/**************************************************************
*                                                             *
*                   Sockets                                   *
*                                                             *
**************************************************************/


int     PlatformSendSocketNonBlocking ( int socket, const unsigned char *buf, SIZE_TYPE len, int flags )
{
    int ret;
    int ret2;
    ret = send ( socket,buf,len,flags );
    if ( ret == -1 )
    {
        ret2 = errno;
        if ( ( ret2 == EWOULDBLOCK ) || ( ret2 == EAGAIN ) )
        {
            return CLIENT_SEND_BUFFER_FULL;
        }
        if ( errno == ECONNRESET )
        {
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"ECONNRESET","" );
            return CLIENT_DISCONNECTED;
        }
        if ( errno == EPIPE )
        {
            LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"EPIPE","" );
            return CLIENT_DISCONNECTED;
        }

        LOG ( CONNECTION_LOG,ERROR_LEVEL,socket,"Unhandled Error %d",ret2 );
        return ret;
    }
    return ret;
}
