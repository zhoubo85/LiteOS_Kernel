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
 
#ifndef _LOS_MIP_CONFIG_H
#define _LOS_MIP_CONFIG_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define LOS_MPOOL_PKGBUF_SIZE   1024*4
#define LOS_MPOOL_MSGS_SIZE     512
#define LOS_MPOOL_SOCKET_SIZE   512*2
#define LOS_MPOOL_TCP_SIZE      1024

#define LOS_MIP_TCP_WIND_SIZE       512
#define LOS_MIP_TCP_SND_BUF_SIZE    512
/* mip resource(sockets number, arp table size ...) max size */
#define MIP_RES_SIZE_MAX        6

/* Message queue max messages in queue */
#define MIP_MESG_QUEUE_LENGTH	MIP_RES_SIZE_MAX
/* arp wait queue size */
#define LOS_MIP_MAX_WAIT_CON    MIP_RES_SIZE_MAX
#define MIP_ARP_TAB_SIZE        MIP_RES_SIZE_MAX

#define MIP_CONN_MAX            MIP_RES_SIZE_MAX
#define MIP_CONN_RCVBOX_SIZE    MIP_RES_SIZE_MAX
#define MIP_CONN_ACPTBOX_SIZE   MIP_RES_SIZE_MAX


#define MIP_HW_MTU_VLAUE            1500
/* tcp connection internal timer timeout value */
#define MIP_TCP_INTERNAL_TIMEOUT    200
/* fin_wait_2 default timeout value , it means 10*200ms */
#define MIP_TCP_FIN_WAIT2_TIMEOUT   10
/* persist default timeout value , it means 50*200ms */
#define MIP_TCP_PERSIST_TIMEOUT   50
/* tcp base sycle timer's timeout value, it means 50ms */
#define MIP_TCP_BASE_TIMER_TIMEOUT  50

/* 2MSL default timeout value , it means 300*200ms */
#define MIP_TCP_2MSL_TIMEOUT   300

/* tcp max segement size */
#define MIP_TCP_MSS                 512

/* enable igmp function, 0 means not enable, 1 means enable */
#define MIP_EN_IGMP 1 

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_MIP_CONFIG_H */
