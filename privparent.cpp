//#include"privparent.h"
//#include"privsock.h"
//#include"sysutil.h"
//#include"tunable.h"
//
//static void privop_port_get_data_sock(session_t* sess);
//static void privop_pasv_active(session_t* sess);
//static void privop_pasv_listen(session_t* sess);
//static void privop_pasv_accept(session_t* sess);
//
/////到时可以实验一下能不能直接使用系统调用
//int capset(cap_user_header_t hdr,const cap_user_data_t data){
//    return syscall(__NR_capget,hdr,data);
//}
//
/////用nobody来限制一下权限 并且单独用一个进程来分离出bind<1024端口的权限 挺好的
//void minimize_privilege(){
//    struct passwd* pw=getpwnam("nobody");
//    if(pw==NULL)return;
//    ///把有效用户id改成nobody的
//    if(setegid(pw->pw_gid)<0)ERR_EXIT("setegid");
//    if(seteuid(pw->pw_uid)<0)ERR_EXIT("seteuid");
//
//    struct __user_cap_header_struct hdr;
//    struct __user_cap_data_struct dat;
//    bzero(&hdr,sizeof(hdr)),bzero(&dat,sizeof(dat));
//    hdr.version=_LINUX_CAPABILITY_VERSION_2;
//    hdr.pid=0;
//    __u32 cap_mask=0;
//    ///把绑定<1024端口的权限带上
//    cap_mask|=(1<<CAP_NET_BIND_SERVICE);
//    dat.effective=dat.permitted=cap_mask;
//    dat.inheritable=0;
//    capset(&hdr,&dat);
//}
//
//void handle_parent(session_t* sess){
//    minimize_privilege();
//    char cmd;
//    for(;;){
//        ///感觉在这里读到read return0时exit比较好 写在函数里面不太好
//        cmd=priv_sock_get_cmd(sess->parent_fd);
//        if(cmd==PRIV_SOCK_GET_DATA_SOCK){
//            ///这个应该是主动模式下20端口去connect客户端的数据端口
//            privop_port_get_data_sock(sess);
//        }else if(cmd==PRIV_SOCK_PASV_ACTIVE){
//            privop_pasv_active(sess);
//        }else if(cmd==PRIV_SOCK_PASV_LISTEN){
//            privop_pasv_listen(sess);
//        }else if(cmd==PRIV_SOCK_PASV_ACCEPT){
//            privop_pasv_accept(sess);
//        }
//    }
//}
//
//
//static void privop_port_get_data_sock(session_t* sess){
//    unsigned short port=(unsigned short)priv_sock_get_int(sess->parent_fd);
//    char ip[16]={0};
//    priv_sock_recv_buf(sess->parent_fd,ip,sizeof(ip));
//
//    printf("收到主动模式地址 port:%u ip:",port);
//    for(int i=0;i<16;i++)printf("%c",ip[i]);
//    puts("");
//
//    struct sockaddr_in addr;
//    bzero(&addr,sizeof(addr));
//    addr.sin_family=AF_INET;
//    addr.sin_port=htons(port);
//    addr.sin_addr.s_addr=inet_addr(ip);
//    int fd=tcp_client(0);
//    if(fd==-1){
//        printf("主动模式 socket bad\n");
//        priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
//        return ;
//    }
//    ///select return 0 似乎只是没有可连接的
//    if(connect_timeout(fd,&addr,tunable_connect_timeout)<0){
//        close(fd);
//        priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
//        return;
//    }
//    printf("##priv par  主动 data socket:%d\n",fd);
//    priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_OK);
//    priv_sock_send_fd(sess->parent_fd,fd);
//    close(fd);
//
//}
//
/////判断pasv 监听是否开启
//static void privop_pasv_active(session_t* sess){
//    int active=sess->pasv_listen_fd!=-1?1:0;
//    priv_sock_send_int(sess->parent_fd,active);
//}
//
//static void privop_pasv_listen(session_t* sess){
//    char ip[16]={0};
//    getlocalip(ip);
//    sess->pasv_listen_fd=tcp_server(ip,0);
//    struct sockaddr_in addr;
//    socklen_t len=sizeof(addr);
//    if(getsockname(sess->pasv_listen_fd,(struct sockaddr*)&addr,&len)<0){
//        ERR_EXIT("getsockname");
//    }
//    unsigned short port=ntohs(addr.sin_port);
//    priv_sock_send_int(sess->parent_fd,(int)port);
//}
//
//static void privop_pasv_accept(session_t* sess){
//    int fd=accept_timeout(sess->pasv_listen_fd,NULL,tunable_accept_timeout);
//    close(sess->pasv_listen_fd);
//    sess->pasv_listen_fd=-1;
//    if(fd==-1){
//        priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_BAD);
//        return;
//    }
//    priv_sock_send_result(sess->parent_fd,PRIV_SOCK_RESULT_OK);
//    priv_sock_send_fd(sess->parent_fd,fd);
//    close(fd);
//}
//
//
//
//
//
