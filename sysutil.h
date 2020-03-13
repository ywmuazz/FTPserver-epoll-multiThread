#ifndef SYSUTIL_H_INCLUDED
#define SYSUTIL_H_INCLUDED

#include "common.h"

///启动一个要连接客户端的socket,一般是port
int tcp_client(unsigned short port);
int tcp_server(const char* host,unsigned short port);

int getlocalip(char* ip);

void activate_nonblock(int fd);
void deactivate_nonblock(int fd);

int read_timeout(int fd,unsigned wait_seconds);
int write_timeout(int fd,unsigned wait_seconds);
int accept_timeout(int fd,sockaddr_in *addr,unsigned wait_seconds);
int connect_timeout(int fd,struct sockaddr_in* addr,unsigned wait_seconds);

ssize_t readn(int fd,void* buf,size_t count);
ssize_t writen(int fd,const void* buf,size_t count);
ssize_t recv_peek(int sockfd,void* buf,size_t len);
ssize_t readline(int sockfd,void* buf,size_t maxline);

void send_fd(int sock_fd,int fd);
int recv_fd(const int sock_fd);

const char* statbuf_get_perms(struct stat* sbuf);
const char* statbuf_get_date(struct stat* sbuf);

int lock_file_read(int fd);
int lock_file_write(int fd);
int unlock_file(int fd);

long get_time_sec();
long get_time_usec();
void nano_sleep(double seconds);

//带外数据
void activate_oobinline(int fd);
void activate_sigurg(int fd);

char* get_file_name (const int fd);
std::string getFN (const int fd);


#endif // SYSUTIL_H_INCLUDED
