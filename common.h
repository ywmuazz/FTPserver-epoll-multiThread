#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#define _XOPEN_SOURCE
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<pwd.h>
#include<shadow.h>
#include"crypt.h"
#include<fcntl.h>
#include<errno.h>
#include<signal.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<linux/capability.h>
#include<sys/syscall.h>
#include<sys/sendfile.h>
#include<pthread.h>
#include<string>

#include<time.h>
#include<sys/stat.h>
#include<dirent.h>
#include<sys/time.h>

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>

#define  ERR_EXIT(msg) \
    do{ \
        perror(msg); \
        exit(EXIT_FAILURE); \
    }while(0)


#define MAX_COMMAND_LINE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024
#define MYFTP_CONF "myftpd.conf"
#define MAX_SESSIONS 1200
#define MAXEVENTNUM 80
#define TIMEOUTSEG 2
#define TOOMANYCONNTOT -1
#define TOOMANYCONNPERIP -2
#define STATUS_NONE 0
#define STATUS_WRITE 1
#define STATUS_READ 2
#define STATUS_RM 3

#endif // COMMON_H_INCLUDED
