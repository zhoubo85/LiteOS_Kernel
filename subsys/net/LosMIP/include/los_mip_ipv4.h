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
 
#ifndef _LOS_MIP_IPV4_H
#define _LOS_MIP_IPV4_H

#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipaddr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MIP_IP_RF 0x8000U        /* reserved fragment flag */
#define MIP_IP_DF 0x4000U        /* don't fragment flag */
#define MIP_IP_MF 0x2000U        /* more fragments flag */
#define MIP_IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */

#define MIP_IP_V4 0x4
#define MIP_IPH(x) (((x)&0xf0)>>4)
#define MIP_IPHLEN(x) ((x)&0x0f)

#define MIP_PRT_ICMP    1
#define MIP_PRT_IGMP    2
#define MIP_PRT_UDP     17
#define MIP_PRT_TCP     6

#define MIP_IP_MIN_HLEN 20
#define MIP_4BYTE_ALIGN(val) (((val) + 3) & ~3)

#define MIP_IPV4_NOOPT_VL 0x45

#define los_mip_ip_equal(addr1, addr2) ((addr1)->addr == (addr2)->addr)
#define los_mip_ip_addrany(addr1) ((addr1)->addr == MIP_IP_ANY)

/* The IP header for ipv4 */
PACK_STRUCT_BEGIN
struct ipv4_hdr 
{
    PACK_STRUCT_FIELD(u8_t vhl);        /* version and header length */
    PACK_STRUCT_FIELD(u8_t tos);        /* type of service */
    PACK_STRUCT_FIELD(u16_t len);       /* total length */
    PACK_STRUCT_FIELD(u16_t id);        /* identification */ 
    PACK_STRUCT_FIELD(u16_t offset);    /* fragment offset field */
    PACK_STRUCT_FIELD(u8_t ttl);        /* time to live */ 
    PACK_STRUCT_FIELD(u8_t proto);      /* protocol */
    PACK_STRUCT_FIELD(u16_t chksum);    /* checksum */
    PACK_STRUCT_FIELD(struct ipv4_addr src);     /* source IP addresses */
    PACK_STRUCT_FIELD(struct ipv4_addr dest);    /* destination IP addresses */ 
};
PACK_STRUCT_END

int los_mip_ipv4_input(struct netbuf *p, struct netif *dev);
int los_mip_make_ipaddr(ip_addr_t *addr, u32_t seg1, 
                        u32_t seg2, u32_t seg3,u32_t seg4);
int los_mip_ipv4_output(struct netif *dev, struct netbuf *p, 
                        ip_addr_t *src, ip_addr_t *dest,
                        u8_t ttl, u8_t tos,
                        u8_t proto, u8_t *opt, u8_t optlen);
int los_mip_is_multicast(struct ipv4_addr *address);
struct netif *los_mip_ip_route(ip_addr_t *dst);
u32_t los_mip_checksum(unsigned char *buf, int len);
u32_t los_mip_checksum_pseudo_ipv4(struct netbuf *p, 
                                u8_t proto, 
                                u16_t tlen, 
                                ip_addr_t *src, 
                                ip_addr_t *dest);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_IPV4_H */
