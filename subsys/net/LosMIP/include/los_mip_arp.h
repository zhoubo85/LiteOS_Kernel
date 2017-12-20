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
 
#ifndef _LOS_MIP_ARP_H
#define _LOS_MIP_ARP_H

#include <stdlib.h>
#include <string.h>

#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"
#include "los_mip_ipaddr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MIP_ARP_HEADER_LEN 28

#define MIP_MULTICAST_ADDR_0 0x01
#define MIP_MULTICAST_ADDR_1 0x00
#define MIP_MULTICAST_ADDR_2 0x5e
#define MIP_MULTICAST_MASK 0x7f

/* ARP hwtype values */
enum mip_arp_hwtype 
{
    TYPE_ETH = 1//only support ethernet now
};

/* ARP message types (opcodes) */
enum mip_arp_opcode 
{
    ARP_REQ = 1,
    ARP_REP = 2,
    ARP_MAX
};


PACK_STRUCT_BEGIN
struct mip_arp_hdr 
{
    PACK_STRUCT_FIELD(u16_t hwtype;)
    PACK_STRUCT_FIELD(u16_t proto;)
    PACK_STRUCT_FIELD(u8_t  hwlen;)
    PACK_STRUCT_FIELD(u8_t  protolen;)
    PACK_STRUCT_FIELD(u16_t opcode;)
    PACK_STRUCT_FIELD(struct eth_mac sndmac;)
    PACK_STRUCT_FIELD(struct ipv4_addr sndip;)
    PACK_STRUCT_FIELD(struct eth_mac dstmac;)
    PACK_STRUCT_FIELD(struct ipv4_addr dstip;)
};
PACK_STRUCT_END

struct arp_table
{
    struct eth_mac hw;
    struct ipv4_addr ip;
    u32_t tm;
};

int los_mip_arp_request(struct netif *dev, ip_addr_t *ipaddr);
int los_mip_arp_xmit(struct netif *dev, struct netbuf *p, 
                     ip_addr_t *dstip);
int los_mip_arp_input(struct netif *dev, struct netbuf *p);
int los_mip_arp_tab_init(void);
int los_mip_find_arp_entry(struct ipv4_addr *ip, 
                           struct eth_mac **dstmac);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_ARP_H */
