#ifndef __MY_HEADER__
#define __MY_HEADER__
 
#define UNIXSTR_PATH "/tmp/unix.str"
 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <errno.h> /* for the EINTR constant */
#include <sys/wait.h> /* for the waitpid() system call */
#include <sys/un.h> /* for Unix domain sockets */
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>  
#include <signal.h>
#include <time.h>
#include <pthread.h>
 
#endif
