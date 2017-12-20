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
 
#ifndef _LOS_MIP_IGMP_H
#define _LOS_MIP_IGMP_H

#include "los_mip_typed.h"
#include "los_mip_ipv4.h"
#include "los_mip_osfunc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define IGMP_ALL_GROUP  0xE0000001U
#define IGMP_ALL_ROUTER 0xE0000002U

#define IGMP_MEMB_QUERY   0x11 /* igmp member query message */
#define IGMP_MEMB_REPORT1 0x12 /* igmp member report message for version 1 */
#define IGMP_MEMB_REPORT2 0x16 /* igmp member report message for version 2 */
#define IGMP_MEMB_LEAVE   0x17 /* igmp member leave message for version 2, version 1 don't have leave message */
#define IGMP_MEMB_REPORT3 0x22 /* igmp member report message for version 3 */

#define IGMP_PKG_V12_SIZE 8 /* igmp version 1 and 2 message length */
 
#define IGMP_DEFAULT_DELAY_TIMEOUT 500 /* means 500 milliseconds */

#define IGMP_STATE_NONE         0x00 /* Init state */
#define IGMP_STATE_IDLE         0x01 /* nothing is processing */
#define IGMP_STATE_TMR_RUNNING  0x02 /* group respone delay timer is running */

#define IGMP_TTL    1
#define RALERT      0x9404U /* router alert option value */
#define RALERT_LEN  4

PACK_STRUCT_BEGIN
struct igmp_msgv12 
{
    PACK_STRUCT_FIELD(u8_t    type);        /* igmp message type, it's compatable with version 1 */
    PACK_STRUCT_FIELD(u8_t    reserved);    /* reserved */
    PACK_STRUCT_FIELD(u16_t   checksum);    /* igmp checksum */
    PACK_STRUCT_FIELD(struct ipv4_addr group_addr); /* igmp group ip address */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END


struct mip_igmp_group 
{
    /* multicast address */
    struct ipv4_addr    address;
    /* timer id for reporting */
    mip_timer_t         timer;
    /* indicate if the timer is running */
    u8_t                state;
    /* counter of the group address used by local task */
    u8_t                ref;
    /* next link */
    struct mip_igmp_group *next;
};

int los_mip_igmp_input(struct netbuf *p, struct netif *dev, 
                       ip_addr_t *src, ip_addr_t *dst);
int los_mip_igmp_join_group(ip_addr_t *ipaddr, ip_addr_t *groupaddr);
int los_mip_igmp_leave_group(ip_addr_t *ipaddr, ip_addr_t *groupaddr);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_IGMP_H */
