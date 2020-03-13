#ifndef TUNABLE_H_INCLUDED
#define TUNABLE_H_INCLUDED

#include "common.h"

extern int tunable_pasv_enable;
extern int tunable_port_enable;
extern unsigned tunable_listen_port;
extern unsigned tunable_max_clients;
extern unsigned tunable_max_per_ip;
extern unsigned tunable_accept_timeout;
extern unsigned tunable_connect_timeout;
extern unsigned tunable_idle_session_timeout;
extern unsigned tunable_data_connection_timeout;
extern unsigned tunable_local_umask;
extern unsigned tunable_upload_max_rate;
extern unsigned tunable_download_max_rate;
extern const char* tunable_listen_address;


#endif // TUNABLE_H_INCLUDED
