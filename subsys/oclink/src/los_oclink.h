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
 
#ifndef _LOS_OCLINK_H
#define _LOS_OCLINK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "los_mip.h"
#include "coap_core.h"
#include "los_coap.h"

#define OCLINK_OK               0
#define OCLINK_NG               1

#define OCLINK_IOTS_ADDR        0
#define OCLINK_IOTS_PORT        1
#define OCLINK_IOTS_ADDRPORT    2
#define OCLINK_IOT_DEVSN        3
#define OCLINK_CMD_CALLBACK     4
#define OCLINK_MAX              5

#define OCLINK_MAX_SN_LEN       16
#define OCLINK_MAX_ADDR_LEN     16

#define OCLINK_BIND_TIMEOUT     4000 /* ms */

/* 
    oclink internel running state
    INIT--->BINDING
    BINDING--->WAIT_BINDOK
    WAIT_BINDOK ----> IDLE
    IDLE ---> IDLE
    IDLE ---> ERR
    ERR ---> BINDING
    
*/
#define OCLINK_STAT_INIT        0
#define OCLINK_STAT_BINDING     1
#define OCLINK_STAT_WAIT_BINDOK 2
#define OCLINK_STAT_IDLE        3
#define OCLINK_STAT_ERR         4
#define OCLINK_STAT_MAX         5



#define OCLINK_ERR_CODE_NODATA      1 /* can't get udp data over 2 scconds. */
#define OCLINK_ERR_CODE_NETWORK     2 /* may be server address err casuse this err. */
#define OCLINK_ERR_CODE_BINDTIMEOUT 3 /* bind process timeout(over 5 seconds), need redo bind process. */
#define OCLINK_ERR_CODE_DEVNOTEXSIT 4 /* device(identify by the sn) not registed to the platform, or your sn. */
#define OCLINK_TASK_STACK_SIZE      1024 /* oclink task size */
#define OCLINK_TASK_PRIO            16   /* oclink task priority */

typedef char oclink_param_t;

typedef int(*cmdcallback)(char *cmd, int len);
int los_oclink_get_errcode(void);

int los_oclink_set_param(oclink_param_t kind, void *data, int len);
int los_oclink_send(char *buf, int len);
int los_oclink_start(void);
int los_oclink_stop(void);

#endif
