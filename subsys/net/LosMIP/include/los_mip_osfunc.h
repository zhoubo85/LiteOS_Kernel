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
 
#ifndef _LOS_MIP_OSFUNC_H
#define _LOS_MIP_OSFUNC_H

#include "los_event.h"
#include "los_membox.h"
#include "los_hwi.h"
#include "los_mux.ph"
#include "los_queue.ph"
#include "los_sem.ph"
#include "los_swtmr.ph"
#include "los_sys.ph"
#include "los_task.ph"
#include "los_tick.ph"

#include "los_mip_config.h"
#include "los_mip_typed.h"

#define MIP_INVALID_MBOX    0
#define MIP_MAX_DELAY       0xffffffffUL
#define MIP_OS_TIMEOUT      0xffffffffUL

#define MIP_INVALID_TIMER   OS_SWTMR_MAX_TIMERID


typedef UINT32 mip_mbox_t;
typedef UINT32 mip_sem_t;
typedef UINT32 mip_mutex_t;
typedef UINT32 mip_thread_t;
typedef UINT16 mip_timer_t;

typedef int mip_prot_t;
typedef void (*mip_thread_fn)(void *arg);

int los_mip_mbox_new(mip_mbox_t *mbox, int size);
int los_mip_mbox_free(mip_mbox_t *mbox);
int los_mip_mbox_post(mip_mbox_t *mbox, void *data);
int los_mip_mbox_trypost(mip_mbox_t *mbox, void *msg);
int los_mip_mbox_fetch(mip_mbox_t *mbox, 
                       void **msg, u32_t timeout);
int los_mip_mbox_tryfetch(mip_mbox_t *mbox, void **msg);
int los_mip_mbox_valid(mip_mbox_t *mbox);
void los_mip_mbox_set_invalid(mip_mbox_t *mbox);
int los_mip_sem_new(mip_sem_t *sem, u16_t count);
u32_t los_mip_sem_wait(mip_sem_t *sem, u32_t timeout);
void los_mip_sem_signal(mip_sem_t *sem);
void los_mip_sem_free(mip_sem_t *sem);
int los_mip_sem_valid(mip_sem_t *sem);
void los_mip_sem_set_invalid(mip_sem_t *sem);
void los_mip_osfunc_init(void);
int los_mip_mutex_new(mip_mutex_t *mutex);
void los_mip_mutex_free(mip_mutex_t *mutex);
void los_mip_mutex_lock(mip_mutex_t *mutex);
void los_mip_mutex_unlock(mip_mutex_t *mutex);
mip_thread_t los_mip_thread_new(char *name, mip_thread_fn thread , 
                                void *arg, int stacksize, int prio);
void los_mip_thread_delete(mip_thread_t thread_id);

mip_prot_t los_mip_task_lock(void);
void los_mip_task_unlock(mip_prot_t pval);
u32_t los_mip_cur_tm(void);
u32_t los_mip_random(void);
void los_mip_delay(u32_t ms);
mip_timer_t los_mip_create_swtimer(u32_t timeout, 
                                   SWTMR_PROC_FUNC callback, 
                                   u32_t args);
int los_mip_is_valid_timer(mip_timer_t id);
int los_mip_start_timer(mip_timer_t id);
int los_mip_stop_timer(mip_timer_t id);
int los_mip_delete_timer(mip_timer_t id);
#endif /* _LOS_MIP_OSFUNC_H */

