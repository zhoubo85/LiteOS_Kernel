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
 
#ifndef _LOS_MIP_UDP_H
#define _LOS_MIP_UDP_H

#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipv4.h"
#include "los_mip_ipaddr.h"
#include "los_mip_netbuf.h"
#include "los_mip_connect.h"


PACK_STRUCT_BEGIN
struct udp_hdr {
  PACK_STRUCT_FIELD(u16_t src);
  PACK_STRUCT_FIELD(u16_t dest);  /* src/dest UDP ports */
  PACK_STRUCT_FIELD(u16_t len);
  PACK_STRUCT_FIELD(u16_t chksum);
};
PACK_STRUCT_END

#define MIP_UDP_HDR_LEN 8

#define MIP_DHCP_SER_PORT 0x0043
#define MIP_DHCP_CLI_PORT 0x0044

struct udp_ctl;
/* @addr the remote IP address from which the packet was received */
typedef void (*udp_recv_func)(void *arg, struct udp_ctl *pcb, 
                              struct netbuf *p, 
                              const ip_addr_t *addr, u16_t port);


struct udp_ctl
{
    struct udp_ctl *next;
    ip_addr_t localip;
    u16_t lport;            /* local port */
    ip_addr_t remote_ip; 
    u16_t rport;            /*remote port */
    u8_t so_opt;            /* Socket options */ 
    u8_t tos;               /* Type Of Service */           
    u8_t ttl;               /* Time To Live */
    udp_recv_func rcvfn;
    void *rcvarg;
};


int los_mip_udp_input(struct netbuf *p, struct netif *dev, ip_addr_t *src, ip_addr_t *dst);
int los_mip_udp_output( struct udp_ctl *udpctl, struct netbuf *p, ip_addr_t *dst_ip, u16_t dst_port);


#endif
