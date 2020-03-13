#include "common.h"
#include "sysutil.h"
#include "session.h"
#include "str.h"
#include "tunable.h"
#include "parseconf.h"
#include "ftpproto.h"
#include "ftpcodes.h"
#include "hash.h"
#include "csapp.h"
#include "epoll_util.h"

extern session_t *p_sess;
static unsigned s_children;
static hash_t *s_ip_count_hash;
static hash_t *s_pid_ip_hash;
int epollfd;

int isValid(session_t* sess);
void handle_sigchld(int sig);
unsigned hash_func(unsigned buckets,void *key);
unsigned handle_ip_count(void* ip);
void drop_ip_count(void* ip);
int handleNewConn(int connfd,struct sockaddr_in*);
int processRead(edata_t* sess);
int processWrite(edata_t* sess);
int closeSess(session_t*);

//session_t* sessions[MAX_SESSIONS]= {0};
edata_t sessinfo[MAX_SESSIONS]= {0};
toNode* map_fd_toNode[MAX_SESSIONS];
int linkMRU(toNode* node);
int unlinkNode(toNode* node);
int refreshTimeout(int fd);
int addtoNode(session_t* sess);
int epoll_wait_timeout(int epfd, struct epoll_event* events, int maxevents);

toNode* pMRU=NULL;
toNode* pLRU=NULL;




int main() {

//    int ddd=open(".",O_RDONLY);
//
//    printf("ddd:%d\n%s\n",ddd,get_file_name(ddd));//getFN(ddd).c_str()
//    int sss=openat(ddd,"..",O_RDONLY);
//    printf("sss:%d\n%s\n",sss,get_file_name(sss));

    parseconf_load_file(MYFTP_CONF);
    //daemon(0,0); ///真正启动的时候在弄守护进程

    printf("tunable_pasv_enable=%d\n", tunable_pasv_enable);
    printf("tunable_port_enable=%d\n", tunable_port_enable);

    printf("tunable_listen_port=%u\n", tunable_listen_port);
    printf("tunable_max_clients=%u\n", tunable_max_clients);
    printf("tunable_max_per_ip=%u\n", tunable_max_per_ip);
    printf("tunable_accept_timeout=%u\n", tunable_accept_timeout);
    printf("tunable_connect_timeout=%u\n", tunable_connect_timeout);
    printf("tunable_idle_session_timeout=%u\n", tunable_idle_session_timeout);
    printf("tunable_data_connection_timeout=%u\n", tunable_data_connection_timeout);
    printf("tunable_local_umask=0%o\n", tunable_local_umask);
    printf("tunable_upload_max_rate=%u\n", tunable_upload_max_rate);
    printf("tunable_download_max_rate=%u\n", tunable_download_max_rate);

    if (tunable_listen_address == NULL)
        printf("tunable_listen_address=NULL\n");
    else
        printf("tunable_listen_address=%s\n", tunable_listen_address);


    if(getuid()!=0) {
        fprintf(stderr,"myftpd: must be started as root.\n");
        exit(EXIT_FAILURE);
    }

    session_t sess=init_session;

    p_sess=&sess;

    sess.bw_upload_rate_max=tunable_upload_max_rate;
    sess.bw_download_rate_max=tunable_download_max_rate;

    s_ip_count_hash=hash_alloc(256,hash_func);
    s_pid_ip_hash=hash_alloc(256,hash_func);

    signal(SIGCHLD,handle_sigchld);
    ///监听cmd连接的socket
    int listenfd=tcp_server(tunable_listen_address,tunable_listen_port);
    struct sockaddr_in addr;
    socklen_t scklen=sizeof(sockaddr);

    epoll_event events[MAXEVENTNUM];
    epollfd=Epoll_create(80);

    fd_t* fdt=(fd_t*)malloc(sizeof(fd_t));
    fdt->fd=listenfd;
    fdt->type=0;
    sessinfo[listenfd]= {fdt,NULL};
    addFdData(epollfd,listenfd,&sessinfo[listenfd],false);
    int allcnt=0;


    for(;;) {
        int eventnum=epoll_wait_timeout(epollfd,events,MAXEVENTNUM);
        for(int i=0; i<eventnum; i++) {
            edata_t* pedat=(edata_t*)(events[i].data.ptr);
            int sockfd=pedat->fd_in->fd;
            session_t* sess=pedat->sess;
            if(sockfd==listenfd) {
                printf("epoll 返回监听到有连接\n");
                for(;;) {
                    int connfd=accept(listenfd,(struct sockaddr*)&addr,&scklen);
                    if(connfd==-1) {
                        if(errno==EAGAIN||errno==EWOULDBLOCK)break;
                        unix_error("accept error.\n");
                    }
                    ///new添加指针 在里面add监听
                    if(handleNewConn(connfd,&addr)==0) {
                        allcnt++;
                    } else {
                        printf("建立连接失败.\n");
                    }
                    printf("监听事件数:%d\n",allcnt);

                }
            } else if(events[i].events&EPOLLIN) {
                refreshTimeout(sockfd);
                int res=processRead(pedat);
                if(res==STATUS_WRITE)
                    modfd(epollfd,sockfd,EPOLLOUT);
                else if(res==STATUS_RM)
                    removefd(epollfd,sockfd);
            } else if(events[i].events&EPOLLOUT) {
                int res=processWrite(pedat);
                if(res==STATUS_READ)
                    modfd(epollfd,sockfd,EPOLLIN);
                else if(res==STATUS_RM)
                    removefd(epollfd,sockfd);
            } else if(events[i].events&EPOLLHUP) {
                closeSess(sess);
            }

        }
    }
    return 0;
}

int sessTime(toNode* node) {
    return get_time_sec()-node->lastTime;
}

bool isTimeout(toNode* node) {
    return sessTime(pLRU)+TIMEOUTSEG>tunable_idle_session_timeout;
}
int removeToNode(toNode* rmNode) {
    map_fd_toNode[rmNode->sess->ctrl_fd]=NULL;
    closeSess(rmNode->sess);
    unlinkNode(rmNode);
    free(rmNode);
    return 0;
}

///认为只要返回0个事件，即是超时无事件,返回>0个事件，即为未超时就返回事件.
///不认为在超时的时间点会恰好有事件返回.
int epoll_wait_timeout(int epfd, struct epoll_event* events, int maxevents) {
//    Epoll_wait(epollfd,events,MAXEVENTNUM,);
    if(pLRU==NULL) {
        return Epoll_wait(epfd,events,maxevents,-1);
    } else {
        int waitTime=tunable_idle_session_timeout-sessTime(pLRU);
        int num=Epoll_wait(epfd,events,maxevents,waitTime);
        if(num<0)return num;
        else if(num>0)return num;
        else {
            while(pLRU!=NULL&&isTimeout(pLRU)) {
                removeToNode(pLRU);
            }
            return 0;
        }
    }
}


int linkMRU(toNode* node) {
    node->right=pMRU;
    node->left=NULL;
    if(pMRU) {
        pMRU->left=node;
    }
    pMRU=node;
    if(pLRU==NULL)pLRU=node;
    return 0;
}

int unlinkNode(toNode* node) {
    if(node->left) {
        node->left->right=node->right;
    } else {
        pMRU=node->right;
    }
    if(node->right) {
        node->right->left=node->left;
    } else {
        pLRU=node->left;
    }
    node->left=node->right=NULL;

    return 0;
}


int refreshTimeout(int fd) {
    if(map_fd_toNode[fd]==NULL) {
        printf("hashtable error\n");
        return -1;
    }
    toNode* node=map_fd_toNode[fd];
    node->lastTime=get_time_sec();
    unlinkNode(node);
    linkMRU(node);
    return 0;
}

void beginSession(session_t* sess) {
    ///把这个命令socket设置为可接受带外数据
//    activate_oobinline(sess->ctrl_fd);

    handleChild(sess);
}

///返回0为成功
int handleNewConn(int connfd,struct sockaddr_in* addr) {
    ///0估计就是没有等待时间,直接阻塞?

    unsigned ip=addr->sin_addr.s_addr;
    session_t* sess=(session_t*)malloc(sizeof(session_t));

    *sess=init_session;

    sess->num_clients=s_children;
    sess->ctrl_fd=connfd;

    int rc=isValid(sess);
    if(rc<0)return rc;

    s_children++;
    sess->num_this_ip=handle_ip_count(&ip);

    fd_t* fdt=(fd_t*)malloc(sizeof(fd_t));
    fdt->fd=connfd;
    fdt->type=FDTYPECTRL;

    sessinfo[connfd]= {fdt,sess};

//    addfd(epollfd,connfd,0);
    addFdData(epollfd,connfd,&sessinfo[connfd],0);
    beginSession(sess);

    addtoNode(sess);

    printf("有客户端连接到来\n");

    return 0;
}



int addtoNode(session_t* sess) {
    long sec=get_time_sec();
    toNode* node=newToNode(sess,sec);
    linkMRU(node);
    map_fd_toNode[sess->ctrl_fd]=node;
    return 0;
}


int isValid(session_t* sess) {
    if(tunable_max_clients>0&&sess->num_clients>tunable_max_clients) {
        ftp_reply(sess,FTP_TOO_MANY_USERS,"There are too many users. Try later.");
        closeSess(sess);
        return TOOMANYCONNTOT;
    }
    if(tunable_max_per_ip>0&&sess->num_this_ip>tunable_max_per_ip) {
        ftp_reply(sess,FTP_IP_LIMIT,"There are too many connections from your IP. Try later.");
        closeSess(sess);
        return TOOMANYCONNPERIP;
    }
    return 0;
}

void handle_sigchld(int sig) {
    pid_t pid;
    while((pid=waitpid(-1,NULL,WNOHANG))>0) {
        s_children--;
        unsigned *ip=(unsigned*)hash_lookup_entry(s_pid_ip_hash,&pid,sizeof(pid));
        if(ip==NULL)continue;
        drop_ip_count(ip);
        hash_free_entry(s_pid_ip_hash,&pid,sizeof(pid));
    }
}


unsigned hash_func(unsigned buckets,void* key) {
    return *(unsigned*)key%buckets;
}

unsigned handle_ip_count(void* ip) {
    unsigned cnt;
    unsigned *pcnt=(unsigned*)hash_lookup_entry(s_ip_count_hash,ip,sizeof(unsigned));
    if(pcnt==NULL) {
        cnt=1;
        hash_add_entry(s_ip_count_hash,ip,sizeof(unsigned),&cnt,sizeof(unsigned));
    } else {
        cnt=*pcnt;
        ++cnt;
        *pcnt=cnt;
    }
    return cnt;
}

void drop_ip_count(void* ip) {
    unsigned ret;
    unsigned *pcnt=(unsigned*)hash_lookup_entry(s_ip_count_hash,(void*)ip,sizeof(unsigned));
    if(pcnt==NULL)return;
    ret=*pcnt;
    ///感觉这种情况是没有可能发生的，因为这个函数是在中断处理程序中调用的
    ///但是中断处理程序没用奇技淫巧，所以它不会被中断，所以这个函数也不会被中断
    ///即减成0和删除entry是一气呵成的
    if(ret<=0)return;
    --ret;
    *pcnt=ret;
    if(ret==0)hash_free_entry(s_ip_count_hash,(void*)ip,sizeof(unsigned));
}
















