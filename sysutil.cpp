#include "sysutil.h"

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

///返回一个用于connect 对面client 的socket,(主动模式下规定必须用20端口连对面client)
int tcp_client(unsigned short port) {
    int sock;
    if((sock=socket(AF_INET,SOCK_STREAM,0))<0)ERR_EXIT("tcp_client");
    if(port>0) {
        int on=1;
        if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&on,sizeof(on))<0)
            ERR_EXIT("setsockopt");
        char ip[16]= {0};
        getlocalip(ip);
        struct sockaddr_in addr;
        bzero(&addr,sizeof(addr));
        addr.sin_family=AF_INET;
        addr.sin_port=htons(port);
        addr.sin_addr.s_addr=inet_addr(ip);
        if(bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0)
            ERR_EXIT("bind??");
    }
    return sock;
}

///被动模式下只需要随便开一个>=1024的端口监听数据连接即刻
int tcp_server(const char* host,unsigned short port) {
    int listenfd;
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0)
        ERR_EXIT("tcp_server");
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    if(host) {
        struct hostent* ht=gethostbyname(host);
        if(ht==NULL)ERR_EXIT("gethostbyname");
        servaddr.sin_addr=*(struct in_addr*)ht->h_addr;
    } else servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(port);

    //虽然pasv随便开端口 但是为了保险还是复用一下
    int on=1;
    if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&on,sizeof(on))<0)
        ERR_EXIT("setsockopt");
    if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
        ERR_EXIT("bind??");
    if(listen(listenfd,SOMAXCONN)<0)
        ERR_EXIT("listen");
    return listenfd;
}

int getlocalip(char* ip) {
    char host[100]= {0};
    if(gethostname(host,sizeof(host))<0)return -1;
    struct hostent* ht;
    if((ht=gethostbyname(host))==NULL)return -1;
    strcpy(ip,inet_ntoa(*(struct in_addr*)(ht->h_addr)));
    return 0;
}

void activate_nonblock(int fd) {
    int ret;
    int flag=fcntl(fd,F_GETFL);
    if(flag==-1)ERR_EXIT("fcntl");
    flag|=O_NONBLOCK;
    ret=fcntl(fd,F_SETFL,flag);
    if(ret==-1)ERR_EXIT("fcntl");
}

void deactivate_nonblock(int fd) {
    int ret;
    int flag=fcntl(fd,F_GETFL);
    if(flag==-1)ERR_EXIT("fcntl");
    flag&=~O_NONBLOCK;
    ret=fcntl(fd,F_SETFL,flag);
    if(ret==-1)ERR_EXIT("fcntl");
}

///在限定时间内检测是否可读(但不读)，传入0表示阻塞
///可读返回0 不可读或出错其他返回-1，如果是超时不可读errno还会为ETIMEDOUT 注意这是个系统自带宏 意思是连接超时
///但是传入0并不会阻塞的检测是否可读?? 这个设计??
int read_timeout(int fd,unsigned waitsec) {
    int ret=0;
    ///select是不能阻塞的 要么限时 要么立即返回
    if(waitsec>0) {
        fd_set st;
        struct timeval timeout;
        FD_ZERO(&st);
        FD_SET(fd,&st);
        timeout.tv_sec=waitsec;
        timeout.tv_usec=0;

        ///select 是慢系统调用 被中断则停止阻塞，需要检测到这种因中断导致的错误
        do {
            ret=select(fd+1,&st,NULL,NULL,&timeout);
        } while(ret<0&&errno==EINTR);

        if(ret<0)ERR_EXIT("select");

        if(ret==0) {
            ret=-1;
            errno=ETIMEDOUT;
        } else if(ret==1) {
            ret=0;
        }
    }
    return ret;
}

///检测限定时间内是否可写但不写
///可写返回0 出错-1 不可写-1且errno=ETIMEDOUT

int write_timeout(int fd,unsigned waitsec) {
    int ret=0;
    if(waitsec>0) {
        fd_set st;
        struct timeval timeout;
        FD_ZERO(&st);
        FD_SET(fd,&st);
        timeout.tv_sec=waitsec;
        timeout.tv_usec=0;
        ///为什么src放在了err的位置?
        do {
            ret=select(fd+1,NULL,&st,NULL,&timeout);
        } while(ret<0&&errno==EINTR);

        if(ret<0)ERR_EXIT("select");
        if(ret==0) {
            ret=-1;
            errno=ETIMEDOUT;
        } else if(ret==1)ret=0;
    }
    return ret;
}


///尝试在限定时间内accept一个连接 返回sockid 无连接到了返回-1 errno=ETIMEDOUT
///这里是真的调用accept,如果waitsec为0表示阻塞
int accept_timeout(int fd,struct sockaddr_in* addr,unsigned waitsec) {
    int ret=0;
    socklen_t len=sizeof(sockaddr);
    if(waitsec>0) {
        fd_set st;
        struct timeval timeout;
        FD_ZERO(&st);
        FD_SET(fd,&st);
        timeout.tv_sec=waitsec;
        timeout.tv_usec=0;
        ///为什么src放在了err的位置?
        do {
            ret=select(fd+1,&st,NULL,NULL,&timeout);
        } while(ret<0&&errno==EINTR);
        if(ret<0)return -1;
        else if(ret==0) {
            errno=ETIMEDOUT;
            return -1;
        }
    }
    ret=accept(fd,(struct sockaddr*)addr,&len);
    return ret;
}

int connect_timeout(int fd,struct sockaddr_in* addr,unsigned waitsec) {
    int ret;
    if(waitsec>0)
        activate_nonblock(fd);
    ret=connect(fd,(struct sockaddr*)addr,sizeof(struct sockaddr));
    if(ret<0&&errno==EINPROGRESS) {
        fd_set st;
        struct timeval timeout;
        FD_ZERO(&st);
        FD_SET(fd,&st);
        timeout.tv_sec=waitsec;
        timeout.tv_usec=0;
        ///一旦检测到连接已建立就可写 当然返回错误信息也可写
        do {
            ret=select(fd+1,NULL,&st,NULL,&timeout);
        } while(ret<0&&errno==EINTR);
        if(ret==0) {
            ///因为后面还要取消非阻塞 所以先不return
            ret=-1;
            errno=ETIMEDOUT;
        } else if(ret<0) {
            return -1;
        } else if(ret==1) {
            ///所以这里还要检测是因为错误返回 还是 因为连接建立
            int err;
            socklen_t len=sizeof(err);
            int sockopt=getsockopt(fd,SOL_SOCKET,SO_ERROR,&err,&len);
            if(sockopt==-1)return -1;
            else if(err==0)ret=0;
            else {
                errno=err;
                ret=-1;
            }
        }

    }
    if(waitsec>0)deactivate_nonblock(fd);
    return ret;
}

///ssize_t是有符号的size类型 字节数与操作系统有关 看看是多少位的机器 3232 6464
///读n个字节 成功返回cnt 失败返回-1  读到eof返回实际读取的字节数
ssize_t readn(int fd,void* buf,size_t cnt) {
    size_t rest=cnt;
    ssize_t nread;
    char* rbuf=(char*)buf;
    while(rest>0) {
        if((nread=read(fd,rbuf,rest))<0) {
            if(errno==EINTR)continue;
            return -1;
        } else if(nread==0)return cnt-rest; //EOF
        rbuf+=nread;
        rest-=nread;
    }
    return cnt;
}


ssize_t writen(int fd,const void* buf,size_t cnt) {
    size_t rest=cnt;
    ssize_t nwrite;
    char* wbuf=(char*)buf;
    while(rest>0) {
        if((nwrite=write(fd,wbuf,rest))<0) {
            if(errno==EINTR)continue;
            return -1;
        } else if(nwrite==0)continue;
        wbuf+=nwrite;
        rest-=nwrite;
    }
    return cnt;
}

///读取缓冲区的数据 但不移出
ssize_t recv_peek(int fd,void* buf,size_t len) {
    for(;;) {
        int ret=recv(fd,buf,len,MSG_PEEK);
        if(ret==-1&&errno==EINTR)continue;
        return ret;
    }
}

///成功返回>=0 失败返回-1
///现在改掉了 成功返回读取的总字节数  为0可以判断为eof
ssize_t readline(int fd,void* buf,size_t len) {
    int ret,nread,rest=len,tot=0;
    char* rbuf=(char*)buf;
    for(;;) {
        nread=ret=recv_peek(fd,rbuf,rest);
        ///出错或eof直接返回
        if(ret<=0)return ret;
        char *p=strchr(rbuf,'\n');
        if(p) {
            ret=readn(fd,rbuf,p-rbuf+1);
            if(ret!=p-rbuf+1)ERR_EXIT("readline");
            tot+=ret;
            return tot;
        }
        ///'\n'没找到 但是如果读它就会爆缓冲区
        if(nread>rest)ERR_EXIT("readline");
        ret=readn(fd,rbuf,nread);
        if(ret!=nread)ERR_EXIT("readline");
        rbuf+=nread;
        rest-=nread;
        tot+=nread;
    }
    return -1;
}



void send_fd(int sock_fd,int fd) {
    int ret;
    struct msghdr msg;
    struct cmsghdr *p_cmsg;
    struct iovec vec;

    char cmsgbuf[CMSG_SPACE(sizeof(fd))];

    int *p_fds;
    char sendchar = 0;

    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    p_cmsg = CMSG_FIRSTHDR(&msg);
    p_cmsg->cmsg_level = SOL_SOCKET;
    p_cmsg->cmsg_type = SCM_RIGHTS;
    p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    p_fds = (int*)CMSG_DATA(p_cmsg);
    *p_fds = fd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;

    vec.iov_base = &sendchar;
    vec.iov_len = sizeof(sendchar);
    ret = sendmsg(sock_fd, &msg, 0);
    if (ret != 1)
        ERR_EXIT("sendmsg");
}

int recv_fd(const int sock_fd) {
    int ret;
    struct msghdr msg;
    char recvchar;
    struct iovec vec;
    int recv_fd;
    char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
    struct cmsghdr *p_cmsg;
    int *p_fd;
    vec.iov_base = &recvchar;
    vec.iov_len = sizeof(recvchar);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    msg.msg_flags = 0;

    p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
    *p_fd = -1;
    ret = recvmsg(sock_fd, &msg, 0);
    if (ret != 1)
        ERR_EXIT("recvmsg");

    p_cmsg = CMSG_FIRSTHDR(&msg);
    if (p_cmsg == NULL)
        ERR_EXIT("no passed fd");


    p_fd = (int*)CMSG_DATA(p_cmsg);
    recv_fd = *p_fd;
    if (recv_fd == -1)
        ERR_EXIT("no passed fd");

    return recv_fd;
}


///文件权限转成字符串
const char* statbuf_get_perms(struct stat *sbuf) {
    static char perms[] = "----------";
    perms[0] = '?';

    mode_t mode = sbuf->st_mode;
    switch (mode & S_IFMT) {
    case S_IFREG:
        perms[0] = '-';
        break;
    case S_IFDIR:
        perms[0] = 'd';
        break;
    case S_IFLNK:
        perms[0] = 'l';
        break;
    case S_IFIFO:
        perms[0] = 'p';
        break;
    case S_IFSOCK:
        perms[0] = 's';
        break;
    case S_IFCHR:
        perms[0] = 'c';
        break;
    case S_IFBLK:
        perms[0] = 'b';
        break;
    }

    if (mode & S_IRUSR) {
        perms[1] = 'r';
    }
    if (mode & S_IWUSR) {
        perms[2] = 'w';
    }
    if (mode & S_IXUSR) {
        perms[3] = 'x';
    }
    if (mode & S_IRGRP) {
        perms[4] = 'r';
    }
    if (mode & S_IWGRP) {
        perms[5] = 'w';
    }
    if (mode & S_IXGRP) {
        perms[6] = 'x';
    }
    if (mode & S_IROTH) {
        perms[7] = 'r';
    }
    if (mode & S_IWOTH) {
        perms[8] = 'w';
    }
    if (mode & S_IXOTH) {
        perms[9] = 'x';
    }
    if (mode & S_ISUID) {
        perms[3] = (perms[3] == 'x') ? 's' : 'S';
    }
    if (mode & S_ISGID) {
        perms[6] = (perms[6] == 'x') ? 's' : 'S';
    }
    if (mode & S_ISVTX) {
        perms[9] = (perms[9] == 'x') ? 't' : 'T';
    }

    return perms;
}


const char* statbuf_get_date(struct stat *sbuf) {
    static char datebuf[64] = {0};
    const char *p_date_format = "%b %e %H:%M";
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t local_time = tv.tv_sec;
    if (sbuf->st_mtime > local_time || (local_time - sbuf->st_mtime) > 60*60*24*182) {
        p_date_format = "%b %e  %Y";
    }
    struct tm* p_tm = localtime(&local_time);
    strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);
    return datebuf;
}



static int lock_internal(int fd,int lock_type) {
    int ret;
    struct flock lk;
    bzero(&lk,sizeof(struct flock));
    lk.l_type=lock_type;
    lk.l_whence=SEEK_SET;
    lk.l_start=lk.l_len=0; //len=0意思整个锁上
    do {
        ret=fcntl(fd,F_SETLKW,&lk);///LKW意思是阻塞
    } while (ret<0&&errno==EINTR);
    return ret;
}


int lock_file_read(int fd) {
    return lock_internal(fd,F_RDLCK);
}

int lock_file_write(int fd) {
    return lock_internal(fd,F_WRLCK);
}

int unlock_file(int fd) {
    int ret;
    struct flock lk;
    bzero(&lk,sizeof(lk));
    lk.l_type=F_UNLCK;
    lk.l_whence=SEEK_SET;
    lk.l_start=lk.l_len=0;
    ret=fcntl(fd,F_SETLK,&lk);
    return ret;
}


static struct timeval s_curr_time;

int getTimeStamp(long& sec,long& usec){
    if(gettimeofday(&s_curr_time,NULL)<0)ERR_EXIT("gettimeofday");
    sec=s_curr_time.tv_sec;
    usec=s_curr_time.tv_usec;
    return 0;
}

long get_time_sec() {
    if(gettimeofday(&s_curr_time,NULL)<0)ERR_EXIT("gettimeofday");
    return s_curr_time.tv_sec;
}

long get_time_usec() {
    return s_curr_time.tv_usec;
}

///timespec tv_nsec是纳秒 1e-9
void nano_sleep(double seconds) {
    time_t secs=(time_t)seconds;
    double frac=seconds-(double)secs;
    struct timespec ts;
    ts.tv_sec=secs;
    ts.tv_nsec=(long)(frac*1000000000);
    int ret;
    do {
        ret=nanosleep(&ts,&ts);
    } while(ret==-1&&errno==EINTR);
}

// 开启套接字fd接收带外数据的功能
void activate_oobinline(int fd) {
    int t=1;
    int ret=setsockopt(fd,SOL_SOCKET,SO_OOBINLINE,&t,sizeof(t));
    if(ret==-1)ERR_EXIT("setsockopt");
}


///fd上有带外数据到来时会产生SIGURG，这里激活之后该进程才能接受到这个fd的sigurg信号
void activate_sigurg(int fd) {
    int ret=fcntl(fd,F_SETOWN,getpid());
    if(ret==-1)ERR_EXIT("fcntl");
}


char* get_file_name (const int fd) {
    if (0 >= fd)
        return NULL;
    char buf[1024]={0};
    static char file_path[1024]={0};
    bzero(file_path,sizeof(file_path));
    snprintf(buf, sizeof (buf), "/proc/self/fd/%d", fd);
    if (readlink(buf, file_path, sizeof(file_path) - 1) != -1)
        return file_path;
    return NULL;
}

std::string getFN (const int fd)
{
    if (0 >= fd) {
        return std::string ();
    }

    char buf[1024] = {'\0'};
    char file_path[PATH_MAX] = {'0'}; // PATH_MAX in limits.h
    snprintf(buf, sizeof (buf), "/proc/self/fd/%d", fd);
    if (readlink(buf, file_path, sizeof(file_path) - 1) != -1) {
        return std::string (file_path);
    }

    return std::string ();
}










