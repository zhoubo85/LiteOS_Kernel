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
 
#ifndef _LOS_MIP_IPADDR_H
#define _LOS_MIP_IPADDR_H

#include "los_mip_typed.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MIP_ETH_MAC_LEN 6
#define MIP_IP_ANY ((u32_t)0x00000000)
#define MIP_IP_BROADCAST    ((u32_t)0xffffffffUL)

/* Get one byte from the 4-byte address */
#define MIP_ADDR1(ipv4addr) (((u8_t*)(ipv4addr))[0])
#define MIP_ADDR2(ipv4addr) (((u8_t*)(ipv4addr))[1])
#define MIP_ADDR3(ipv4addr) (((u8_t*)(ipv4addr))[2])
#define MIP_ADDR4(ipv4addr) (((u8_t*)(ipv4addr))[3])

#define MIP_IP_DEFAULT_TTL 0x7F

PACK_STRUCT_BEGIN
struct ipv4_addr 
{
    PACK_STRUCT_FIELD(u32_t addr;)
};
PACK_STRUCT_END

PACK_STRUCT_BEGIN
struct eth_mac 
{
    PACK_STRUCT_FIELD(u8_t hwaddr[MIP_ETH_MAC_LEN];)
};
PACK_STRUCT_END

typedef struct ipv4_addr ip_addr_t;


#define IP_V4_ADDR_SIZE 4
#define IP_V6_ADDR_SIZE 16

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_IPADDR_H */
