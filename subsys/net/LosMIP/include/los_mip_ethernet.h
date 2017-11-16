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
 
#ifndef _LOS_MIP_ETHERNET_H
#define _LOS_MIP_ETHERNET_H

#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"

#define MIP_ETH_HW_LEN 6
#define MIP_ETH_HDR_LEN 14

enum mip_eth_types 
{
    /** ipv4 type */
    TYPE_IPV4        = 0x0800U,
    /** arp type */
    TYPE_ARP       = 0x0806U, 
    /** ipv6 type */
    TYPE_IPV6      = 0x86DDU,
    TYPE_MAX
};

PACK_STRUCT_BEGIN
struct eth_hdr
{
    PACK_STRUCT_FIELD(struct eth_mac dst;)
    PACK_STRUCT_FIELD(struct eth_mac src;)
    PACK_STRUCT_FIELD(u16_t type;)
};
PACK_STRUCT_END

int los_mip_eth_arp_output(struct netif *dev, 
                           struct netbuf *p,
                           struct eth_mac *src, 
                           struct eth_mac *dst);
int los_mip_eth_ip_output(struct netif *dev, 
                          struct netbuf *p, 
                          struct eth_mac *src, 
                          struct eth_mac *dst,
                          enum mip_eth_types type);
int los_mip_eth_input(struct netbuf *p, struct netif *dev);

u16_t los_mip_get_ip_ver(struct netbuf *p);
int los_mip_set_ip_ver(struct netbuf *p, u16_t ver);



#endif
