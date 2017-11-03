#ifndef _LOS_MIP_H
#define _LOS_MIP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "los_mip_netif.h"
#include "los_mip_tcpip_core.h"
#include "los_mip_ethif.h"
#include "los_mip_ipaddr.h"
#include "los_mip_ipv4.h"
#include "los_mip_socket.h"
#include "los_mip_inet.h"

struct udp_res_t
{
    int fd;/* socket fd */ 
    struct sockaddr_in remoteAddr;
};


#endif
