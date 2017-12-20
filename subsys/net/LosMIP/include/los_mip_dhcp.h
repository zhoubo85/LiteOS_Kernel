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
 
#ifndef _LOS_MIP_DHCP_H
#define _LOS_MIP_DHCP_H

#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"
#include "los_mip_ethernet.h"
#include "los_mip_arp.h"
#include "los_mip_ipv4.h"
#include "los_mip_udp.h"
#include "los_mip_connect.h"
#include "los_mip_socket.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MIP_DHCP_TASK_SIZE 1024
#define MIP_DHCP_TASK_PRIO 8
#define MIP_DHCP_MAX_RETRY 4

#define MIP_DHCP_DISCOVER       1
#define MIP_DHCP_OFFER          2
#define MIP_DHCP_REQUEST        3
#define MIP_DHCP_ACK            4
#define MIP_DHCP_ARP_REQUEST    5
#define MIP_DHCP_RELEASE        6
#define MIP_DHCP_DECLINE        7
#define MIP_DHCP_INFORM         8

#define MIP_MIN_OPT_LEN         68

#define MIP_DHCP_OP_REQUEST     1
#define MIP_DHCP_OP_REPLY       2
#define MIP_DHCP_HW_ETH         1
#define MIP_DHCP_HW_ETH_LEN     6

#define MIP_DHCP_OPT_PAD 0
#define MIP_DHCP_OPT_SUBNET_MASK 1 /* RFC 2132 3.3 */
#define MIP_DHCP_OPT_ROUTER 3
#define MIP_DHCP_OPT_DNS_SERVER 6 
#define MIP_DHCP_OPT_HOSTNAME 12
#define MIP_DHCP_OPT_IP_TTL 23
#define MIP_DHCP_OPT_MTU 26
#define MIP_DHCP_OPT_BROADCAST 28
#define MIP_DHCP_OPT_TCP_TTL 37
#define MIP_DHCP_OPT_END 255

#define MIP_DHCP_OPT_REQUESTED_IP 50 // RFC 2132 9.1, requested IP address  
#define MIP_DHCP_OPT_REQUESTED_IPV4_LEN 4 
#define MIP_DHCP_OPT_LEASE_TIME 51 // RFC 2132 9.2, time in seconds, in 4 bytes  
#define MIP_DHCP_OPT_OVERLOAD 52 // RFC2132 9.3, use file and/or sname field for options  

#define MIP_DHCP_OPT_MSG_TYPE 53 // RFC 2132 9.6, important for DHCP  
#define MIP_DHCP_OPT_MSG_TYPE_LEN 1

#define MIP_DHCP_OPT_SERVER_ID 54 // RFC 2132 9.7, server IP address  
#define MIP_DHCP_OPT_PARAMETER_REQUEST_LIST 55 // RFC 2132 9.8, requested option types  
#define MIP_DHCP_OPT_PARAMETER_REQUEST_LIST_LEN 4 

#define MIP_DHCP_OPT_MAX_MSG_SIZE 57 // RFC 2132 9.10, message size accepted >= 576  
#define MIP_DHCP_OPT_MAX_MSG_SIZE_LEN 2

#define MIP_DHCP_OPT_T1 58 // T1 renewal time */
#define MIP_DHCP_OPT_T2 59 // T2 rebinding time */
#define MIP_DHCP_OPT_US 60
#define MIP_DHCP_OPT_CLIENT_ID 61
#define MIP_DHCP_OPT_CLIENT_ID_ETH_LEN 7
#define MIP_DHCP_OPT_TFTP_SERVERNAME 66
#define MIP_DHCP_OPT_BOOTFILE 67

#define MIP_DHCP_MAGIC_COOKIE 0x63538263

#define MIP_DHCP_STATE_PROCESSING   1 // dhcp is working
#define MIP_DHCP_STATE_FINISHED     2 // dhcp already done
#define MIP_DHCP_STATE_ERR          3 // dhcp error happened
#define MIP_DHCP_STATE_NONE         0 // dhcp not started

#define MIP_DHCP_ERR_TIMEOUT        1 // no dhcp server, or no ack message come
#define MIP_DHCP_ERR_ADDR_CONFLICT  2 // address conflict
#define MIP_DHCP_ERR_NONE           0 // no err

typedef int (*dhcp_callback)(u8_t err, u8_t state);

PACK_STRUCT_BEGIN
struct dhcp_ipv4_info
{
    PACK_STRUCT_FIELD(u8_t opcode);
    PACK_STRUCT_FIELD(u8_t hwtype);  // src/dest UDP ports 
    PACK_STRUCT_FIELD(u8_t hwlen);
    PACK_STRUCT_FIELD(u8_t hops);
    PACK_STRUCT_FIELD(u32_t transid);
    PACK_STRUCT_FIELD(u16_t second);
    PACK_STRUCT_FIELD(u16_t flags);
    PACK_STRUCT_FIELD(u32_t clientip);
    PACK_STRUCT_FIELD(u32_t yourip);
    PACK_STRUCT_FIELD(u32_t serverip);
    PACK_STRUCT_FIELD(u32_t gwip);
    PACK_STRUCT_FIELD(u8_t clienmac[16]);
    PACK_STRUCT_FIELD(u8_t servernam[64]);
    PACK_STRUCT_FIELD(u8_t files[128]);
    PACK_STRUCT_FIELD(u32_t magiccookie);
    PACK_STRUCT_FIELD(u8_t options[MIP_MIN_OPT_LEN]);
};
PACK_STRUCT_END


int los_mip_dhcp_start(struct netif *ethif, dhcp_callback func );
int los_mip_dhcp_stop(void);
int los_mip_dhcp_wait_finish(struct netif *ethif);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_DHCP_H */
