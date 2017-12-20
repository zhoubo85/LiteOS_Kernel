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
 
#ifndef _LOS_MIP_ERR_H
#define _LOS_MIP_ERR_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MIP_OK          0
#define MIP_ERR_PARAM   1

#define MIP_POOL_INIT_FAILED    22
#define MIP_MEM_FREE_FAILED     23
#define MIP_MPOOL_MALLOC_FAILED 24
#define MIP_MPOOL_JUMP_LEN_ERR  25


#define MIP_OSADP_MEM_NULL          31
#define MIP_OSADP_FREE_FAILED       32
#define MIP_OSADP_MSGPOST_FAILED    33
#define MIP_OSADP_SEM_CREAT_FAILED  34
#define MIP_OSADP_TIMEOUT           35
#define MIP_OSADP_MSGPOST_INVALID_ARG   36
#define MIP_OSADP_MSGPOST_QUEUE_FULL    37
#define MIP_OSADP_QUEUE_EMPTY           38
#define MIP_OSADP_OSERR                 39

#define MIP_ETH_ERR_USE     41
#define MIP_ETH_ERR_PARAM   42

#define MIP_ARP_NOT_FOUND       51
#define MIP_ARP_QUE_FULL        52
#define MIP_ARP_SEND_FAILED     53
#define MIP_ARP_WAIT_LIST_FULL  54
#define MIP_ARP_NEED_REQ_ARP    55

#define MIP_IP_HEAD_VER_ERR     61
#define MIP_IP_LEN_ERR          62
#define MIP_IP_HDR_CHECKSUM_ERR 63
#define MIP_IP_NOT_SUPPORTED    64
#define MIP_IP_NOT_FOR_US       65

#define MIP_UDP_PKG_LEN_ERR     71
#define MIP_UDP_PKG_CHKSUM_ERR  72

#define MIP_TCP_PKG_LEN_ERR     81
#define MIP_TCP_PKG_CHKSUM_ERR  82
#define MIP_TCP_TMR_OPS_ERR     83
#define MIP_TCP_LISTEN_MBOX_ERR 84
#define MIP_TCP_SEQ_NUM_ERR     85

#define MIP_ICMP_PKG_LEN_ERR    91
#define MIP_ICMP_PKG_CHKSUM_ERR 92

#define MIP_IGMP_PKG_LEN_ERR    96
#define MIP_IGMP_PKG_CHKSUM_ERR 97
#define MIP_IGMP_TMR_START_ERR  98

#define MIP_CON_NEW_MSG_FAILED      101
#define MIP_CON_PORT_ALREADY_USED   102
#define MIP_CON_PORT_RES_USED_OUT   103

#define MIP_SOCKET_ERR              111
#define MIP_SOCKET_NOT_SUPPORTED    112
#define MIP_SOCKET_SND_TIMEOUT      113


#define MIP_NOT_SUPPORTED           121

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_ERR_H */
