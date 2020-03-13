#include"common.h"
#include"session.h"
#include"ftpproto.h"
#include"sysutil.h"

toNode* newToNode(session_t* sess,int t,toNode* l,toNode* r){
    toNode* ret=(toNode*)malloc(sizeof(toNode));
    ret->sess=sess;
    ret->lastTime=t;
    ret->left=l;
    ret->right=r;
    return ret;
}

void begin_session(session_t* sess){
}
