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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#include "los_mip_osfunc.h"
#include "los_mip_typed.h"
#include "los_mip_err.h"

/*****************************************************************************
 Function    : los_mip_mbox_new
 Description : create message queue for mip core process
 Input       : mbox @ message queue handle to store the queue created
               size @ message's count that the queue can store
 Output      : 
 Return      : MIP_OK means success, other failed
 *****************************************************************************/
int los_mip_mbox_new(mip_mbox_t *mbox, int size)
{
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    UINT32 uwRet;

    if (mbox == NULL)
    {
        return -MIP_ERR_PARAM;
    }
    if (size > MIP_MESG_QUEUE_LENGTH)
    {
        /* note: for constrain env only store MIP_MESG_QUEUE_LENGTH messages */
        size = MIP_MESG_QUEUE_LENGTH;
    }
    
    uwRet = LOS_QueueCreate((char *)NULL, (UINT16)(size), 
                            mbox, 0,
                            (UINT16)( sizeof( void * )));
    if (uwRet != LOS_OK)
    {
        *mbox = MIP_INVALID_MBOX;
        return -MIP_OSADP_MEM_NULL;
    }
#endif
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_mbox_free
 Description : destroy message queue 
 Input       : mbox @ message queue handle that will delete
 Output      : 
 Return      : MIP_OK means success, other failed
 *****************************************************************************/
int los_mip_mbox_free(mip_mbox_t *mbox)
{
	int errcnt = 0;

    if((NULL == mbox) || (MIP_INVALID_MBOX == *mbox))
    {
        return -MIP_ERR_PARAM;
    }
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    while (LOS_OK != LOS_QueueDelete((UINT32 )*mbox))
    {
        LOS_TaskDelay(1);
        if (errcnt > 10)
        {
            return -MIP_OSADP_FREE_FAILED;
        }
        errcnt++;
    }
#endif
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_mbox_post
 Description : post a message to the message queue, if the queue is full ,it 
               will block until the the queue read one message out 
 Input       : mbox @ message queue handle
               data @ the message's pointer
 Output      : 
 Return      : MIP_OK means success, other failed
 *****************************************************************************/
int los_mip_mbox_post(mip_mbox_t *mbox, void *data)
{
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    UINT32 uwRet;
    
    if((NULL == mbox) || (*mbox == MIP_INVALID_MBOX))
    {
        return -MIP_ERR_PARAM;
    }
    
    do
    {
        uwRet = LOS_QueueWrite((UINT32)*mbox, (void*)data, 
                               sizeof(UINT32), 
                               LOS_MS2Tick(MIP_MAX_DELAY));
    } while(uwRet != LOS_OK);

#endif
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_mbox_post
 Description : post a message to the message queue, if the queue is full ,it 
               will return right now
 Input       : mbox @ message queue handle to store the queue created
               msg @ message's pointer 
 Output      : None
 Return      : MIP_OK means success, other failed.
 *****************************************************************************/
int los_mip_mbox_trypost(mip_mbox_t *mbox, void *msg)
{
    int ret = 0;
    UINT32 uwRet;
    
    if((NULL == mbox) || (*mbox == MIP_INVALID_MBOX))
    {
        return -MIP_ERR_PARAM;
    }
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    uwRet = LOS_QueueWrite((UINT32)*mbox, (void*)msg, sizeof(UINT32), 0);
    if (uwRet == LOS_OK)
    {
        ret = MIP_OK;
    }
    else if((uwRet == LOS_ERRNO_QUEUE_WRITE_INVALID)
            || (uwRet == LOS_ERRNO_QUEUE_WRITE_IN_INTERRUPT))
    {
        ret = -MIP_OSADP_MSGPOST_INVALID_ARG;
    }
    else if (uwRet == LOS_ERRNO_QUEUE_ISFULL)
    {
       ret = -MIP_OSADP_MSGPOST_QUEUE_FULL;
    }
    else if (uwRet == LOS_ERRNO_QUEUE_TIMEOUT)
    {
        ret = -MIP_OSADP_TIMEOUT;
    }
    else
    {
        ret = -MIP_OSADP_MSGPOST_FAILED;
    }
#endif
    return ret;
}

/*****************************************************************************
 Function    : los_mip_mbox_fetch
 Description : get a message from the message queue, if the queue is empty  
               it will block until data come or timeout
 Input       : mbox @ message queue handle 
               msg @ message's pointer's address
               timeout @ message's pointer's address
 Output      : None
 Return      : MIP_OK means success, other failed.
 *****************************************************************************/
int los_mip_mbox_fetch(mip_mbox_t *mbox, void **msg, u32_t timeout)
{
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    UINT32 uwRet;
    UINT32 addr;
    int ret = MIP_OK;
    
    if ((NULL == mbox) || (*mbox == MIP_INVALID_MBOX) || (NULL == msg))
    {    
        return -MIP_ERR_PARAM;
    }
    uwRet = LOS_QueueRead((UINT32)*mbox, &addr, sizeof(UINT32), LOS_MS2Tick(timeout));
    if (uwRet == LOS_OK)
    {
        //ret.status = osEventMessage;
        *msg = (void *)addr;
    }
    else if ((uwRet == LOS_ERRNO_QUEUE_READ_INVALID) 
             || (uwRet == LOS_ERRNO_QUEUE_READ_IN_INTERRUPT))
    {
        ret = -MIP_ERR_PARAM;
    }
    else if ((uwRet == LOS_ERRNO_QUEUE_ISEMPTY) 
             || (uwRet == LOS_ERRNO_QUEUE_TIMEOUT))
    {
        ret = -MIP_OSADP_TIMEOUT;
    }
    else
    {
        ret = -MIP_OSADP_OSERR;
    }

    return ret;
#endif

}

/*****************************************************************************
 Function    : los_mip_mbox_tryfetch
 Description : get a message from the message queue, if the queue is empty  
               it will return right now
 Input       : mbox @ message queue handle 
               msg @ message's pointer's address
 Output      : None
 Return      : MIP_OK means success, other failed.
 *****************************************************************************/
int los_mip_mbox_tryfetch(mip_mbox_t *mbox, void **msg)
{
    int ret = MIP_OK;
	
    ret = los_mip_mbox_fetch(mbox, msg, 0);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_mbox_valid
 Description : get if the message queue handle is valid 
 Input       : mbox @ message queue handle 
 Output      : None
 Return      : MIP_TRUE means valid message queue handle
 *****************************************************************************/
int los_mip_mbox_valid(mip_mbox_t *mbox)          
{      
    if ((NULL == mbox) || (*mbox == MIP_INVALID_MBOX))
    {
        return MIP_FALSE;
    } 
    else
    {
        return MIP_TRUE;
    }
}
 
/*****************************************************************************
 Function    : los_mip_mbox_set_invalid
 Description : set the message queue handle invalid  
 Input       : mbox @ message queue handle 
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_mbox_set_invalid(mip_mbox_t *mbox)   
{
    *mbox = MIP_INVALID_MBOX;
}

/*****************************************************************************
 Function    : los_mip_mbox_is_empty
 Description : get the message queue if it is empty, unimplemented yet  
 Input       : mbox @ message queue handle 
 Output      : None
 Return      : MIP_TRUE means mbox is empty other means not empty
 *****************************************************************************/
int los_mip_mbox_is_empty(mip_mbox_t *mbox)   
{
    if (NULL == mbox)
    {
        return MIP_TRUE;
    }
    /* not implement yet */
    return MIP_FALSE;
}

/*****************************************************************************
 Function    : los_mip_mbox_is_full
 Description : get the message queue if it is full, unimplemented yet  
 Input       : mbox @ message queue handle 
 Output      : None
 Return      : MIP_TRUE means mbox is full other means not full
 *****************************************************************************/
int los_mip_mbox_is_full(mip_mbox_t *mbox)   
{
    if (NULL == mbox)
    {
        return MIP_TRUE;
    }
    /* not implement yet */
    return MIP_FALSE;
} 

/*****************************************************************************
 Function    : los_mip_sem_new
 Description : create a semphore 
 Input       : sem @ semphore handle 
               count @ the resource count 
 Output      : None
 Return      : MIP_OK means create ok, other means failed.
 *****************************************************************************/
int los_mip_sem_new(mip_sem_t *sem, u16_t count)
{
#if (LOSCFG_BASE_IPC_SEM == YES)
    UINT32 ret;
    
    if (NULL == sem)
    {
        return -MIP_ERR_PARAM;
    }
    ret = LOS_SemCreate(count, sem);
    if(ret != LOS_OK)
    {
        return -MIP_OSADP_SEM_CREAT_FAILED;
    }
#endif
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_sem_wait
 Description : wait a semphore 
 Input       : sem @ semphore handle 
               timeout @ wait timeout value 
 Output      : None
 Return      : elapsed @ elapsed time , if wait failed return MIP_OS_TIMEOUT
 *****************************************************************************/
u32_t los_mip_sem_wait(mip_sem_t *sem, u32_t timeout)
{
#if (LOSCFG_BASE_IPC_SEM == YES)
    uint32_t stime;
    uint32_t etime;
    uint32_t elapsed;
    u32_t ret;
    
    if (NULL == sem)
    {
        return MIP_OS_TIMEOUT;
    }
    
    stime = (UINT32)LOS_TickCountGet();
    if (timeout != 0)
    {
        ret = LOS_SemPend(*sem, timeout);
        if (LOS_OK == ret)
        {
            etime = (UINT32)LOS_TickCountGet();
            elapsed = etime - stime;
            
            return elapsed;
        }
        else
        {
            return MIP_OS_TIMEOUT;
        }
    }
    else
    {
        while (LOS_OK != LOS_SemPend(*sem, LOS_WAIT_FOREVER))
        {
        }
        etime = (UINT32)LOS_TickCountGet();
        elapsed = etime - stime;
        return elapsed;
    }
#endif
}

/*****************************************************************************
 Function    : los_mip_sem_signal
 Description : post a semphore 
 Input       : sem @ semphore handle 
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_sem_signal(mip_sem_t *sem)
{
#if (LOSCFG_BASE_IPC_SEM == YES)
    if (NULL != sem)
    {
        LOS_SemPost(*sem);
    }
#endif
    return ;
}

/*****************************************************************************
 Function    : los_mip_sem_free
 Description : delete a semphore 
 Input       : sem @ semphore handle 
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_sem_free(mip_sem_t *sem)
{
#if (LOSCFG_BASE_IPC_SEM == YES)
    if (NULL != sem)
    {
        LOS_SemDelete(*sem);
    }
#endif
    *sem = NULL;
}

/*****************************************************************************
 Function    : los_mip_sem_free
 Description : judge a semphore if it is valid.
 Input       : sem @ semphore handle 
 Output      : None
 Return      : MIP_FALSE 
 *****************************************************************************/
int los_mip_sem_valid(mip_sem_t *sem)                                               
{
    if (NULL != sem)
    {
        return 0;
    }
    /* unimplement yet */
    return MIP_FALSE; 
                             
}

/*****************************************************************************
 Function    : los_mip_sem_set_invalid
 Description : set a semphore invalid.
 Input       : sem @ semphore handle 
 Output      : None
 Return      : MIP_FALSE 
 *****************************************************************************/                                                                                                                                                      
void los_mip_sem_set_invalid(mip_sem_t *sem)                                        
{                                                                               
    /* unimplemented yet */                                                   
} 

/*****************************************************************************
 Function    : los_mip_osfunc_init
 Description : do some gloable init.now nothing do do.
 Input       : void 
 Output      : None
 Return      : None 
 *****************************************************************************/
void los_mip_osfunc_init(void)
{
    /* todo : some global var init */
}

/*****************************************************************************
 Function    : los_mip_mutex_new
 Description : create a mutex.
 Input       : mutex @ mutex hanle 
 Output      : None
 Return      : MIP_OK means create ok, other value means failed 
 *****************************************************************************/
int los_mip_mutex_new(mip_mutex_t *mutex) 
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    
    if (NULL == mutex)
    {
       return -MIP_ERR_PARAM;
    }

    uwRet = LOS_MuxCreate(mutex);
    if (uwRet != LOS_OK)
    {
        return -MIP_OSADP_MEM_NULL;
    }
#endif
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_mutex_free
 Description : free a mutex.
 Input       : mutex @ mutex hanle 
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_mutex_free(mip_mutex_t *mutex)
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    
    if (NULL != mutex)
    {
        uwRet = LOS_MuxDelete(*mutex);
        if (uwRet != LOS_OK)
        {
            //debug here
            return ;
        }
    }
#endif
    return ;
}

/*****************************************************************************
 Function    : los_mip_mutex_lock
 Description : lock a mutex.
 Input       : mutex @ mutex hanle 
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_mutex_lock(mip_mutex_t *mutex)
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    if (NULL != mutex)
    {
        uwRet = LOS_MuxPend(*mutex, LOS_MS2Tick(MIP_MAX_DELAY));
        if (uwRet != LOS_OK)
        {
            //just for err test
            return ;
        }
    }
#endif
    return ;
}

/*****************************************************************************
 Function    : los_mip_mutex_unlock
 Description : unlock a mutex.
 Input       : mutex @ mutex hanle 
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_mutex_unlock(mip_mutex_t *mutex)
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    if (NULL != mutex)
    {
        uwRet = LOS_MuxPost(*mutex);
        if (uwRet != LOS_OK)
        {
            //just for err test
            return ;
        }
    }
#endif
}

/*****************************************************************************
 Function    : los_mip_thread_new
 Description : create a task
 Input       : name @ task name
               thread @ task process function
               arg @ argument for the task function.
               prio @ priority of the task
 Output      : None
 Return      : taskid @ new task's id
 *****************************************************************************/
mip_thread_t los_mip_thread_new(char *name, 
                                mip_thread_fn thread , 
                                void *arg, 
                                int stacksize, int prio)
{
    mip_thread_t taskid = 0;
    UINT32 uwRet;
    TSK_INIT_PARAM_S stInitParam;
    
    stInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)thread;
    stInitParam.usTaskPrio = prio;
    stInitParam.pcName = name;
    stInitParam.uwStackSize = stacksize;
    stInitParam.auwArgs[0] = (UINT32)arg;
    stInitParam.uwResved   = LOS_TASK_STATUS_DETACHED;

    uwRet = LOS_TaskCreate(&taskid, &stInitParam);
    if (uwRet != LOS_OK)
    {
        //just add for test
        return taskid;
    }
    return taskid;
}

/*****************************************************************************
 Function    : los_mip_thread_delete
 Description : delete a task
 Input       : thread_id @ task's id that need delete.
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_thread_delete(mip_thread_t thread_id)
{
    UINT32  uwRet;

    if (thread_id == NULL)
    {
        return ;
    }
    uwRet = LOS_TaskDelete((UINT32)thread_id);
    
    if (uwRet != LOS_OK)
    {
        //just add for test
        return ;
    }
    return ;
}

/*****************************************************************************
 Function    : los_mip_task_lock
 Description : lock the task, so just run current task.it is used for 
               critical region protection
 Input       : None
 Output      : None
 Return      : ret @ interrupt register's value.
 *****************************************************************************/
mip_prot_t los_mip_task_lock(void)
{
    mip_prot_t ret;

    ret = LOS_IntLock();

    return ret;
}

/*****************************************************************************
 Function    : los_mip_task_unlock
 Description : unlock the task, so all task can be scheduled.
               and restore the interupt register's value
 Input       : pval @ interrupt register's value before lock
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_task_unlock(mip_prot_t pval)
{
    LOS_IntRestore(pval);
}

/*****************************************************************************
 Function    : los_mip_assert
 Description : assert function
 Input       : msg @ unused
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_assert( const char *msg )
{
    (void) msg;
    LOS_IntLock();
    for(;;);
}

/*****************************************************************************
 Function    : los_mip_cur_tm
 Description : get current system time 
 Input       : None
 Output      : None
 Return      : tm @ current time's value
 *****************************************************************************/
u32_t los_mip_cur_tm(void)
{
    u32_t tm = 0;
    tm = LOS_TickCountGet();
    return tm;
}

/*****************************************************************************
 Function    : los_mip_random
 Description : get a random data 
 Input       : None
 Output      : None
 Return      : ret @ random data
 *****************************************************************************/
u32_t los_mip_random(void)
{
	unsigned int ret;
	LOS_TickCountGet();
	srand((unsigned)LOS_TickCountGet());
	ret = rand() % RAND_MAX;
	return ret;
}

/*****************************************************************************
 Function    : los_mip_delay
 Description : delay some time by give arg. 
 Input       : ms @ the time that need delay.(Time unit : Millisecond)
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_delay(u32_t ms)
{
    LOS_TaskDelay(ms);
}

/*****************************************************************************
 Function    : los_mip_create_swtimer
 Description : create a period timer. 
 Input       : timeout @ the timeout value of the soft timer
               callback @ the timeout callback function for the soft timer
 Output      : None
 Return      : id @ the timer id 
 *****************************************************************************/
mip_timer_t los_mip_create_swtimer(u32_t timeout, SWTMR_PROC_FUNC callback, 
                                   u32_t args)
{
    UINT32 uwRet = LOS_OK;
    mip_timer_t id = MIP_INVALID_TIMER;
    
    if (NULL == callback || 0 == timeout)
    {
        return id;
    }
    /* create a period soft timer */
    uwRet = LOS_SwtmrCreate(timeout, LOS_SWTMR_MODE_PERIOD, callback, &id, args);
    if(LOS_OK != uwRet)
    {
        id = MIP_INVALID_TIMER;
    }
    return id;
}

/*****************************************************************************
 Function    : los_mip_is_valid_timer
 Description : judge if the timer is valid. 
 Input       : id @ soft timer id
 Output      : None
 Return      : MIP_TRUE @ timer is valid, MIP_FALSE timer is invalid 
 *****************************************************************************/
int los_mip_is_valid_timer(mip_timer_t id)
{
    if (id == MIP_INVALID_TIMER)
    {
        return MIP_FALSE;
    }
    return MIP_TRUE;
}

/*****************************************************************************
 Function    : los_mip_start_timer
 Description : start a timer by the timer id. 
 Input       : id @ soft timer id
 Output      : None
 Return      : MIP_TRUE @ timer start ok, MIP_FALSE timer start failed 
 *****************************************************************************/
int los_mip_start_timer(mip_timer_t id)
{
    UINT32 uwRet = LOS_OK;
    if (id == MIP_INVALID_TIMER)
    {
        return MIP_FALSE;
    }
    
    uwRet = LOS_SwtmrStart(id);
    if(LOS_OK != uwRet)
    {
        return MIP_FALSE;
    }
    return MIP_TRUE;
}

/*****************************************************************************
 Function    : los_mip_stop_timer
 Description : stop a timer by the timer id. 
 Input       : id @ soft timer id
 Output      : None
 Return      : MIP_TRUE @ timer stop ok, MIP_FALSE timer stop failed 
 *****************************************************************************/
int los_mip_stop_timer(mip_timer_t id)
{
    UINT32 uwRet = LOS_OK;
    
    if (id == MIP_INVALID_TIMER)
    {
        return MIP_FALSE;
    }
    
    uwRet = LOS_SwtmrStop(id);
    if(LOS_OK != uwRet)
    {
        return MIP_FALSE;
    }
    return MIP_TRUE;
}

/*****************************************************************************
 Function    : los_mip_delete_timer
 Description : delete a timer by the timer id. 
 Input       : id @ soft timer id
 Output      : None
 Return      : MIP_TRUE @ timer delete ok, MIP_FALSE timer delete failed 
 *****************************************************************************/
int los_mip_delete_timer(mip_timer_t id)
{
    UINT32 uwRet = LOS_OK;
    
    if (id == MIP_INVALID_TIMER)
    {
        return MIP_FALSE;
    }
    
    uwRet = LOS_SwtmrDelete(id);
    if(LOS_OK != uwRet)
    {
        return MIP_FALSE;
    }
    return MIP_TRUE;
}
