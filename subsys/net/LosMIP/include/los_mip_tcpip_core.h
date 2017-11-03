#ifndef _LOS_MIP_TCPIP_CORE_H
#define _LOS_MIP_TCPIP_CORE_H

#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"

#include "cmsis_os.h"

#define MIP_TCPIP_TASK_STACKSIZE 1024
#define MIP_TCPIP_TASK_PRIO 2
#define MIP_TCPIP_MBOX_SIZE 6

enum msg_type 
{
    TCPIP_MSG_SKT,//socket msg
    TCPIP_MSG_INPKT,//net work input package msg
    TCPIP_MSG_WARP,//net work input package msg
    TCPIP_MSG_DELCON,//delete socket connect msg
    TCPIP_MSG_FDELCON,//force delete socket connect msg
    TCPIP_MSG_MAX
};



struct mip_msg 
{
    enum msg_type type;
    union 
    {
        struct skt_msg *conmsg;//socket 
        struct 
        {
            struct netbuf *p;
            struct netif *netif;
            netif_input_func input_fn;
        } inpkg;
        struct skt_del_msg *delcon;
    } msg;
    
};

struct arp_msg_wait_list
{
    struct mip_msg *msg;
    u32_t storetm;
    struct arp_msg_wait_list *next;
};


int los_mip_tcpip_inpkt(struct netbuf *p, struct netif *inif);
void los_mip_tcpip_init(void *arg);
int los_mip_tcpip_snd_msg(struct mip_msg *msg);

#endif
