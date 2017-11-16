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
 
#include <stdlib.h>
#include <string.h>

#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_ipv4.h"
#include "los_mip_icmp.h"

/*****************************************************************************
 Function    : los_mip_icmp_input
 Description : icmp package process function.
 Input       : dev @ net dev's handle which the data come from
               p @ package's buf pointer
               src @ source ip of the package
               dst @ dest ip of the package
 Output      : None
 Return      : MIP_OK process ok, other value means process failed.
 *****************************************************************************/
int los_mip_icmp_input(struct netbuf *p, 
                       struct netif *dev, 
                       ip_addr_t *src, 
                       ip_addr_t *dst)
{
    struct icmp_hdr *icmph = NULL;
    ip_addr_t srctmp;
    
    if ((NULL == p) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    if (p->len < ICMP_HEADER_LEN)
    {
        netbuf_free(p);
        return -MIP_ICMP_PKG_LEN_ERR;
    }
    icmph = (struct icmp_hdr *)p->payload;
    
    switch (icmph->type)
    {
        case ICMP_TYPE_ECHO:
            icmph->chksum = los_mip_checksum(p->payload, p->len);
            if (icmph->chksum != 0)
            {
                break;
            }
            icmph->type = ICMP_TYPE_ECHO_REPLY;
            srctmp.addr = src->addr;
            los_mip_icmp_echo_reply(p, dev, &srctmp);
            break;
        default:
            break;
    }
    netbuf_free(p);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_icmp_echo_reply
 Description : send out icmp ECHO_REPLY message.
 Input       : dev @ net dev's handle which the data come from
               p @ package's buf pointer
               dst @ dest ip of the package
 Output      : None
 Return      : MIP_OK send ok, other value means send failed.
 *****************************************************************************/
int los_mip_icmp_echo_reply(struct netbuf *p, struct netif *dev, ip_addr_t *dst)
{
    int ret = 0;
    
    ret = los_mip_ipv4_output(dev, 
                              p, &dev->ip_addr, dst,
                              0x40, 0, MIP_PRT_ICMP);
    return ret;
}
