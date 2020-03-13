//#include"privsock.h"
//#include"common.h"
//#include"sysutil.h"
//
//
//void priv_sock_init(session_t* sess){
//    int fd[2];
//    if(socketpair(PF_UNIX,SOCK_STREAM,0,fd)<0)
//        ERR_EXIT("socketpair");
//    sess->parent_fd=fd[0],sess->child_fd=fd[1];
//}
//
//void priv_sock_close(session_t* sess){
//    if(sess->parent_fd!=-1){
//        close(sess->parent_fd);
//        sess->parent_fd=-1;
//    }
//    if(sess->child_fd!=-1){
//        close(sess->child_fd);
//        sess->child_fd=-1;
//    }
//}
//
/////父进程关fd1
//void priv_sock_set_parent_context(session_t* sess){
//    if(sess->child_fd!=-1){
//        close(sess->child_fd);
//        sess->child_fd=-1;
//    }
//}
//
//void priv_sock_set_child_context(session_t* sess){
//    if(sess->parent_fd!=-1){
//        close(sess->parent_fd);
//        sess->parent_fd=-1;
//    }
//}
//
/////fa一个字节的命令
//void priv_sock_send_cmd(int fd,char cmd){
//    if(writen(fd,&cmd,sizeof(cmd))!=sizeof(cmd)){
//        ERR_EXIT("priv_sock_send_cmd");
//    }
//}
//
//char priv_sock_get_cmd(int fd){
//    char ret;
//    int res=readn(fd,&ret,sizeof(ret));
/////知道为什么ret==0就可以认为服务进程结束吗 因为子进程结束时会关掉fd1 ，而close(fd1)就会让这边read0，所有socket都是这样的
//    ///觉得把exit success放在handle_parent那里比较好
//    if(res==0){
//        exit(EXIT_SUCCESS);
//    }else if(res!=sizeof(ret)){
//        ERR_EXIT("priv_sock_get_cmd");
//    }
//    return ret;
//}
//
//
/////nobody发来的result
//void priv_sock_send_result(int fd,char res){
//    if(writen(fd,&res,sizeof(res))!=sizeof(res)){
//        ERR_EXIT("priv_sock_send_result");
//    }
//}
//
//
//char priv_sock_get_result(int fd){
//    char ret;
//    if(readn(fd,&ret,sizeof(ret))!=sizeof(ret)){
//        ERR_EXIT("priv_sock_get_result??");
//    }
//    return ret;
//}
//
//void priv_sock_send_int(int fd,int si){
//    if(writen(fd,&si,sizeof(si))!=sizeof(si)){
//        ERR_EXIT("priv_sock_send_int");
//    }
//}
//
//int priv_sock_get_int(int fd){
//    int ret;
//    if(readn(fd,&ret,sizeof(ret))!=sizeof(ret)){
//        ERR_EXIT("priv_sock_get_int");
//    }
//    return ret;
//}
//
//void priv_sock_send_buf(int fd,const char* buf,unsigned len){
//    ///这个设计感觉就很不统一 很不优美
//    ///这样必须是根据代码设计调用什么函数
//    ///万一之后要增加功能呢?
//    ///尝试改成第一个byte标识后面的数据
//
//    priv_sock_send_int(fd,(int)len);
//    if(len!=writen(fd,buf,len)){
//        ERR_EXIT("priv_sock_send_buf");
//    }
//}
//
//
//void priv_sock_recv_buf(int fd,char* buf,unsigned len){
//    unsigned recvlen=(unsigned)priv_sock_get_int(fd);
//    if(recvlen>len){
//        ERR_EXIT("priv_sock_recv_buf");
//    }
//    if(recvlen!=readn(fd,buf,recvlen)){
//        ERR_EXIT("priv_sock_recv_buf");
//    }
//}
//
//void priv_sock_send_fd(int sock,int fd){
//    send_fd(sock,fd);
//}
//int priv_sock_recv_fd(int sock){
//    return recv_fd(sock);
//}
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
