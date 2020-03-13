#include"common.h"
#include"session.h"
#include"ftpproto.h"
#include"privparent.h"
#include"privsock.h"
#include"sysutil.h"

void begin_session(session_t* sess){
//    ///把这个命令socket设置为可接受带外数据
//    activate_oobinline(sess->ctrl_fd);
//    ///socketpair
//    priv_sock_init(sess);
//    pid_t pid;
//    pid=fork();
//    if(pid<0)ERR_EXIT("fork");
//    if(pid==0){
//        ///服务进程
//        //关fd0
//        priv_sock_set_child_context(sess);
//        handle_child(sess);
//    }else{
//        priv_sock_set_parent_context(sess);
//        handle_parent(sess);
//    }
}
