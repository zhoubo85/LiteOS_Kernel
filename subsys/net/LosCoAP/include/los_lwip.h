#ifndef _LOS_LWIP_H
#define _LOS_LWIP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/tcp.h"
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/sockets.h"


struct udp_res_t
{
    int fd;// socket fd 
    struct sockaddr_in remoteAddr;
};


#endif
