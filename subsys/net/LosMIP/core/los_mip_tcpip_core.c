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
 
#include <stdlib.h>
#include <string.h>

#include "los_mip_err.h" 
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_osfunc.h"
#include "los_mip_ethernet.h"
#include "los_mip_arp.h"
#include "los_mip_connect.h"
#include "los_mip_tcpip_core.h"

/* arp wait list for package that need wait arp done */
static struct arp_msg_wait_list g_arp_w_array[LOS_MIP_MAX_WAIT_CON];
static struct arp_msg_wait_list *g_arp_wmsgs_h = NULL;
static struct arp_msg_wait_list *g_arp_wmsgs_t = NULL;

/* tcp/ip core task wait message timeout value */
#define MIP_MBOX_TIMEOUT 200
static mip_mbox_t g_core_msgbox;

/*****************************************************************************
 Function    : los_mip_init_arp_wlist
 Description : init arp wait arary
 Input       : None
 Output      : None
 Return      : MIP_OK init ok.
 *****************************************************************************/
static int los_mip_init_arp_wlist(void)
{
    memset(g_arp_w_array, 0, LOS_MIP_MAX_WAIT_CON * sizeof(struct arp_msg_wait_list));
    g_arp_wmsgs_h = NULL;
    g_arp_wmsgs_t = NULL;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_wmsgs_add_msg
 Description : add tcp/ip core task msessage to wait list tail
 Input       : msgs @ message's pointer
 Output      : None
 Return      : MIP_OK add ok, other value means add failed.
 *****************************************************************************/
static int los_mip_wmsgs_add_msg(struct mip_msg *msgs)
{
    int i = 0;
    
    if (NULL == msgs)
    {
        return MIP_OK;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i ++)
    {
        if (g_arp_w_array[i].msg == NULL)
        {
            break;
        }
    }
    if (i == LOS_MIP_MAX_WAIT_CON)
    {
        return -MIP_ARP_WAIT_LIST_FULL;
    }
    if (g_arp_wmsgs_h == NULL)
    {
        g_arp_wmsgs_h = &g_arp_w_array[i];
        g_arp_wmsgs_h->msg = msgs;
        g_arp_wmsgs_h->storetm = osKernelSysTick();
        g_arp_wmsgs_t = g_arp_wmsgs_h;
    }
    else
    {
        g_arp_wmsgs_t->next = &g_arp_w_array[i];
        g_arp_wmsgs_t = &g_arp_w_array[i];
        
        g_arp_wmsgs_t->msg = msgs;
        g_arp_wmsgs_t->storetm = osKernelSysTick();
        g_arp_wmsgs_t->next = NULL;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_wmsgs_remove_head
 Description : remove wait list head data , and return the head message
 Input       : None
 Output      : None
 Return      : struct mip_msg * @ wait list head's message pointer
 *****************************************************************************/
static struct mip_msg * los_mip_wmsgs_remove_head(void)
{
    struct mip_msg *msg = NULL;
    
    if (NULL != g_arp_wmsgs_h)
    {
        msg = g_arp_wmsgs_h->msg;
        g_arp_wmsgs_h->msg = NULL;
        g_arp_wmsgs_h->storetm = 0;
        g_arp_wmsgs_h = g_arp_wmsgs_h->next;
    }
    return msg;
}

/*****************************************************************************
 Function    : los_mip_get_wmsgs_head_tm
 Description : get wait list head data's store time.
 Input       : None
 Output      : None
 Return      : g_arp_wmsgs_h->storetm @ the message's stored time.
 *****************************************************************************/
static u32_t los_mip_get_wmsgs_head_tm(void)
{
    if(NULL != g_arp_wmsgs_h)
    {
        return g_arp_wmsgs_h->storetm;
    }
     return 0;   
}

/*****************************************************************************
 Function    : los_mip_tcpip_inpkt
 Description : send a input package message to tcp/ip core task.
 Input       : p @ package that need to be send
               inif @ net dev's pointer which the package comes from.
 Output      : None
 Return      : MIP_OK snd ok, other send failed.
 *****************************************************************************/
int los_mip_tcpip_inpkt(struct netbuf *p, struct netif *inif) 
{
    int ret = 0;
    struct mip_msg *msg = NULL;

    ret = los_mip_mbox_valid(&g_core_msgbox);
    if (!ret)
    {
        return -MIP_ERR_PARAM;
    }
    
    msg = (struct mip_msg *)los_mpools_malloc(MPOOL_MSGS, sizeof(struct mip_msg));
    if (msg == NULL) 
    {
        return -MIP_MPOOL_MALLOC_FAILED;
    }
    memset(msg, 0, sizeof(struct mip_msg));
    msg->type = TCPIP_MSG_INPKT;
    msg->msg.inpkg.p = p;
    msg->msg.inpkg.netif = inif;
    msg->msg.inpkg.input_fn = los_mip_eth_input; 
    if (los_mip_mbox_trypost(&g_core_msgbox, msg) != MIP_OK) 
    {
        los_mpools_free(MPOOL_MSGS, msg);
        msg = NULL;
        return -MIP_OSADP_MSGPOST_FAILED;
    }

    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcpip_snd_msg
 Description : send mip_msg to tcp/ip core task.
 Input       : msg @ mip_msg pointer
 Output      : None
 Return      : MIP_OK snd ok, other send failed.
 *****************************************************************************/
int los_mip_tcpip_snd_msg(struct mip_msg *msg)
{
    if (los_mip_mbox_trypost(&g_core_msgbox, msg) != MIP_OK) 
    {
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_repos_payload
 Description : reset the message's payload position
 Input       : msgs @ mip_msg pointer
               paylaodlen @ the newest payload total length
 Output      : None
 Return      : None
 *****************************************************************************/
static void los_mip_repos_payload(struct mip_msg *msgs, u32_t paylaodlen)
{
    u32_t offset = 0;
    
    if (NULL == msgs)
    {
        return ;
    }
    offset = msgs->msg.conmsg->data.p->len - paylaodlen;
    msgs->msg.conmsg->data.p->payload = (u8_t *)msgs->msg.conmsg->data.p->payload + offset;
    msgs->msg.conmsg->data.p->len = paylaodlen;
}

/*****************************************************************************
 Function    : los_mip_tcpip_task
 Description : tcp/ip core task process function.
               deal with all message input and out here.
 Input       : arg @ not used
 Output      : None
 Return      : None
 *****************************************************************************/
static void los_mip_tcpip_task(void *arg)
{
    u32_t ret = 0;
    int retfn = 0;
    uint32_t msgtm = 0;
    uint32_t curtm = 0;
    u32_t payload_len = 0;
    struct mip_msg *msgs = NULL;
    struct mip_msg *rsndmsgs = NULL;
    
    while (1)
    {
        curtm = osKernelSysTick();
        msgtm = los_mip_get_wmsgs_head_tm();
        if ((msgtm != 0)
            && ((curtm - msgtm) >= 20))
        {
            rsndmsgs = los_mip_wmsgs_remove_head();
            if (NULL != rsndmsgs)
            {
                if (rsndmsgs->msg.conmsg->data.retry >= MIP_CONN_MSG_RESEND_MAX)
                {
                    los_mip_sem_signal(rsndmsgs->msg.conmsg->data.op_finished);
                }
                else
                {
                    retfn = los_mip_tcpip_snd_msg(rsndmsgs);
                    if (retfn != MIP_OK)
                    {
                        /* todo: need set err flag , and tell uplayer send finished */
                        los_mip_sem_signal(rsndmsgs->msg.conmsg->data.op_finished);
                    }
                }
            }
        }

        ret = los_mip_mbox_fetch(&g_core_msgbox, (void **)&msgs, MIP_MBOX_TIMEOUT);
        if (MIP_OK != ret)
        {
            continue; 
        }
        switch (msgs->type)
        {
            case TCPIP_MSG_DELCON:
                los_mip_delay(5);
                msgs->type = TCPIP_MSG_FDELCON;
                while(MIP_OK != los_mip_tcpip_snd_msg(msgs))
                {
                    los_mip_delay(5);
                }

                break;
            case TCPIP_MSG_FDELCON:
                msgs->msg.delcon->sktdelfn(msgs->msg.delcon->con);
                los_mpools_free(MPOOL_MSGS, msgs->msg.delcon);
                msgs->msg.delcon = NULL;
                los_mpools_free(MPOOL_MSGS, msgs);
                msgs = NULL;
                break;
            
            case TCPIP_MSG_INPKT:
                msgs->msg.inpkg.input_fn(msgs->msg.inpkg.p, msgs->msg.inpkg.netif); 
                los_mpools_free(MPOOL_MSGS, msgs);
                msgs = NULL;
                break;
            case TCPIP_MSG_SKT:
                /* recived socket msg , need direct send to hw */
                payload_len = msgs->msg.conmsg->data.p->len;
                retfn = msgs->msg.conmsg->sktsndfn(&msgs->msg.conmsg->data);
                if (MIP_OK != retfn)
                {
                    /* tell socket messages process failed */
                    los_mip_sem_signal(msgs->msg.conmsg->data.op_finished);
                    break;
                }
                if (MIP_TRUE == netbuf_get_if_queued(msgs->msg.conmsg->data.p))
                {
                    netbuf_remove_queued(msgs->msg.conmsg->data.p);
                    /* 
                        netbuf not send directly, we need wait for arp done
                        so we resend the message 
                    */
                    msgs->msg.conmsg->data.retry++;
                    los_mip_wmsgs_add_msg(msgs);
                    los_mip_repos_payload(msgs, payload_len);
                }
                else
                {
                    /* tell up layer send finished */
                    los_mip_sem_signal(msgs->msg.conmsg->data.op_finished);
                }
                break;
            default:
                break;
        }
        msgs = NULL;
    }
    
}

/*****************************************************************************
 Function    : los_mip_tcpip_init
 Description : init tcp/ip task resources and mempool ... 
 Input       : arg @ not uesed
 Output      : 
 Return      : None
 *****************************************************************************/
void los_mip_tcpip_init(void *arg)
{
    los_mpools_init();
    los_mip_init_sockets();
    /* init tcp ip core message box */ 
    if (los_mip_mbox_new(&g_core_msgbox, MIP_TCPIP_MBOX_SIZE) != MIP_OK) 
    {
        /* msg box create failed */
        while(1);
    }
    /* init arp table and wait list */ 
    los_mip_arp_tab_init();
    los_mip_init_arp_wlist();
    /* create tcp/ip data process task */ 
    los_mip_thread_new("tcpipcore", 
                        los_mip_tcpip_task, 
                        NULL, 
                        MIP_TCPIP_TASK_STACKSIZE, 
                        MIP_TCPIP_TASK_PRIO);
    
}
