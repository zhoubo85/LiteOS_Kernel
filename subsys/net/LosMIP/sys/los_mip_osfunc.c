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

/**
 * create message queue for mip core process
 *
 * @param mbox message queue handle to store the queue created
 * @param size message's count that the queue can store
 * @return MIP_OK means success, other failed.
 */
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
    
    uwRet = LOS_QueueCreate((char *)NULL, (UINT16)(size), mbox, 0,(UINT16)( sizeof( void * )));
    if (uwRet != LOS_OK)
    {
        *mbox = MIP_INVALID_MBOX;
        return -MIP_OSADP_MEM_NULL;
    }
#endif
    return MIP_OK;
}

/**
 * destroy message queue 
 *
 * @param mbox message queue handle that will delete
 * @return MIP_OK means success, other failed.
 */
int los_mip_mbox_free(mip_mbox_t *mbox)
{
	int errcnt = 0;

    if(NULL == mbox || MIP_INVALID_MBOX == *mbox)
    {
        return -MIP_ERR_PARAM;
    }
    
    while (LOS_OK != LOS_QueueDelete((UINT32 )*mbox))
    {
        LOS_TaskDelay(1);
        if (errcnt > 10)
        {
            return -MIP_OSADP_FREE_FAILED;
        }
        errcnt++;
    }

    return MIP_OK;
}

/**
 * post a message to the message queue, if the queue is full ,it will block until the
 * the queue read one message out
 *
 * @param mbox message queue handle
 * @param data the message's pointer
 * @return MIP_OK means success, other failed.
 */
int los_mip_mbox_post(mip_mbox_t *mbox, void *data)
{
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    UINT32 uwRet;
    
    if(NULL == mbox || *mbox == MIP_INVALID_MBOX)
    {
        return -MIP_ERR_PARAM;
    }
    
    do
    {
        uwRet = LOS_QueueWrite((UINT32)*mbox, (void*)data, sizeof(UINT32), LOS_MS2Tick(MIP_MAX_DELAY));
    }while(uwRet != LOS_OK);

#endif
    return MIP_OK;
}

/**
 * post a message to the message queue, if the queue is full ,it will return right now
 *
 * @param mbox message queue handle to store the queue created
 * @param msg message's pointer 
 * @return MIP_OK means success, other failed.
 */
int los_mip_mbox_trypost(mip_mbox_t *mbox, void *msg)
{
    int ret = 0;
    UINT32 uwRet;
    
    if(NULL == mbox || *mbox == MIP_INVALID_MBOX)
    {
        return -MIP_ERR_PARAM;
    }
    
    uwRet = LOS_QueueWrite((UINT32)*mbox, (void*)msg, sizeof(UINT32), 0);
    if (uwRet == LOS_OK)
    {
        ret = MIP_OK;
    }
    else if(uwRet == LOS_ERRNO_QUEUE_WRITE_INVALID || uwRet == LOS_ERRNO_QUEUE_WRITE_IN_INTERRUPT)
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
    
    return ret;
}

/**
 * get a message from the message queue, if the queue is empty  
 * it will block until data come or timeout
 * @param mbox message queue handle 
 * @param msg message's pointer's address 
 * @param timeout message's pointer's address 
 * @return MIP_OK means success, other failed.
 */
int los_mip_mbox_fetch(mip_mbox_t *mbox, void **msg, u32_t timeout)
{
#if (LOSCFG_BASE_IPC_QUEUE == YES)
    UINT32 uwRet;
    UINT32 addr;
    int ret = MIP_OK;
    
    if (NULL == mbox || *mbox == MIP_INVALID_MBOX || NULL == msg)
    {    
        return -MIP_ERR_PARAM;
    }
    uwRet = LOS_QueueRead((UINT32)*mbox, &addr, sizeof(UINT32), LOS_MS2Tick(timeout));
    if (uwRet == LOS_OK)
    {
        //ret.status = osEventMessage;
        *msg = (void *)addr;
    }
    else if (uwRet == LOS_ERRNO_QUEUE_READ_INVALID || uwRet == LOS_ERRNO_QUEUE_READ_IN_INTERRUPT)
    {
        ret = -MIP_ERR_PARAM;
    }
    else if (uwRet == LOS_ERRNO_QUEUE_ISEMPTY || uwRet == LOS_ERRNO_QUEUE_TIMEOUT)
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

/**
 * get a message from the message queue, if the queue is empty  
 * it will return right now
 * @param mbox message queue handle 
 * @param msg message's pointer's address 
 * @return MIP_OK means success, other failed.
 */
int los_mip_mbox_tryfetch(mip_mbox_t *mbox, void **msg)
{
    int ret = MIP_OK;
	
    ret = los_mip_mbox_fetch(mbox, msg, 0);
    return ret;
}

 /**
 * get if the message queue handle is valid  
 * 
 * @param mbox message queue handle 
 * @return MIP_TRUE means valid message queue handle
 */  
int los_mip_mbox_valid(mip_mbox_t *mbox)          
{      
    if (NULL == mbox || *mbox == MIP_INVALID_MBOX)
    {
        return MIP_FALSE;
    } 
    else
    {
        return MIP_TRUE;
    }
}

 /**
 * set the message queue handle invalid  
 * 
 * @param mbox message queue handle 
 * @return 
 */ 
void los_mip_mbox_set_invalid(mip_mbox_t *mbox)   
{
    *mbox = MIP_INVALID_MBOX;
}

 /**
 * get the message queue if it is empty, unimplemented yet 
 * 
 * @param mbox message queue handle 
 * @return 
 */
int los_mip_mbox_is_empty(mip_mbox_t *mbox)   
{
    if (NULL == mbox)
    {
        return MIP_TRUE;
    }
    
    return MIP_FALSE;
}

 /**
 * get the message queue if it is full, unimplemented yet 
 * 
 * @param mbox message queue handle 
 * @return 
 */
int los_mip_mbox_is_full(mip_mbox_t *mbox)   
{
    if (NULL == mbox)
    {
        return MIP_TRUE;
    }
    
    return MIP_FALSE;
} 

 /**
 * create a semphore 
 * 
 * @param sem semphore handle 
 * @param count the resource count 
 * @return MIP_OK means create success
 */
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
    if(timeout != 0)
    {
        ret = LOS_SemPend(*sem, timeout);
        if(LOS_OK == ret)
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
        while(LOS_OK != LOS_SemPend(*sem, LOS_WAIT_FOREVER))
        {
        }
        etime = (UINT32)LOS_TickCountGet();
        elapsed = etime - stime;
        return elapsed;
    }
#endif
}

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

int los_mip_sem_valid(mip_sem_t *sem)                                               
{
    if (NULL != sem)
    {
        return 0;
    }    

    return MIP_FALSE; 
                             
}

                                                                                                                                                                
void los_mip_sem_set_invalid(mip_sem_t *sem)                                        
{                                                                               
    /* unimplemented yet */                                                   
} 


// Initialize sys arch
void los_mip_osfunc_init(void)
{
    /* 
        todo : some global var init 
    */
}

int los_mip_mutex_new(mip_mutex_t *mutex) 
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;


    if(NULL == mutex)
    {
       return -MIP_ERR_PARAM;
    }

    uwRet = LOS_MuxCreate(mutex);
    if(uwRet != LOS_OK)
    {
        return -MIP_OSADP_MEM_NULL;
    }
#endif
    return MIP_OK;
}

void los_mip_mutex_free(mip_mutex_t *mutex)
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    if(NULL != mutex)
    {
        uwRet = LOS_MuxDelete(*mutex);
        if(uwRet != LOS_OK)
        {
            //debug here
            return ;
        }
    }
#endif
    return ;
}

void los_mip_mutex_lock(mip_mutex_t *mutex)
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    if (NULL != mutex)
    {
        uwRet = LOS_MuxPend(*mutex, LOS_MS2Tick(MIP_MAX_DELAY));
        if(uwRet != LOS_OK)
        {
            //just for err test
            return ;
        }
    }
#endif
    return ;
}

void los_mip_mutex_unlock(mip_mutex_t *mutex)
{
#if (LOSCFG_BASE_IPC_MUX == YES)
    UINT32  uwRet;
    if(NULL != mutex)
    {
        uwRet = LOS_MuxPost(*mutex);
        if(uwRet != LOS_OK)
        {
            //just for err test
            return ;
        }
    }
#endif
}

mip_thread_t los_mip_thread_new(char *name, mip_thread_fn thread , void *arg, int stacksize, int prio)
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

 /**
 * lock the task, so just run current task.
 * it is used for critical region protection
 * @param void 
 * @return mip_prot_t interrupt status register's value
 */
mip_prot_t los_mip_task_lock(void)
{
    mip_prot_t ret;

    ret = LOS_IntLock();

    return ret;
}

 /**
 * unlock the task, so all task can be scheduled.
 * and restore the interupt register's value
 * @param pval interupt register's value that before locked 
 * @return 
 */
void los_mip_task_unlock(mip_prot_t pval)
{
    LOS_IntRestore(pval);
}

void los_mip_assert( const char *msg )
{
    ( void ) msg;
    LOS_IntLock();
    for(;;);
}

u32_t los_mip_cur_tm(void)
{
    u32_t tm = 0;
    tm = LOS_TickCountGet();
    return tm;
}

u32_t los_mip_random(void)
{
	unsigned int ret;
	LOS_TickCountGet();
	srand((unsigned)LOS_TickCountGet());
	ret = rand() % RAND_MAX;
	return ret;
}

void los_mip_delay(u32_t ms)
{
    LOS_TaskDelay(ms);
}
