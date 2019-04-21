
#ifndef _PLATFORM_INCLUDES_H_
#define _PLATFORM_INCLUDES_H_

#define INET_ADDRSTRLEN 100

#define FIONREAD 1


#define sem_t int

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

#define SHUT_RDWR 1
int shutdown(int socket, int how);


#define SIZE_TYPE size_t
#define TIME_TYPE time_t




#endif
