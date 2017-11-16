/*----------------------------------------------------------------------------
 * Copyright (c) <2017-2017>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of CHN and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/
 
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
