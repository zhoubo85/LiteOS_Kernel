#ifndef _LOS_UNIX_H
#define _LOS_UNIX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/types.h> 
#include<sys/socket.h> 
#include<unistd.h> 
#include<netinet/in.h> 
#include<arpa/inet.h> 
#include<stdio.h> 
#include<stdlib.h> 
#include<errno.h> 
#include<netdb.h> 


struct unix_udp_res_t
{
    int fd;// socket fd 
    struct sockaddr_in remoteAddr;
};


#endif
