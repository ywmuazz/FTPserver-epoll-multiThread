#ifndef FTPPROTO_H_INCLUDED
#define FTPPROTO_H_INCLUDED

#include "session.h"
struct uploadArgs{
    session_t* sess;
    int is_append;
};
void handle_child(session_t* sess);
void ftp_reply(session_t* sess,int status,const char* text);
void handleChild(session_t* sess);


#endif // FTPPROTO_H_INCLUDED
