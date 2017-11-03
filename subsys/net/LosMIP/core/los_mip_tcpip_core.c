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

static struct arp_msg_wait_list g_arp_w_array[LOS_MIP_MAX_WAIT_CON];
static struct arp_msg_wait_list *g_arp_wmsgs_h = NULL;
static struct arp_msg_wait_list *g_arp_wmsgs_t = NULL;

#define MIP_MBOX_TIMEOUT 200
static mip_mbox_t g_core_msgbox;

static int los_mip_init_arp_wlist(void)
{
    memset(g_arp_w_array, 0, LOS_MIP_MAX_WAIT_CON * sizeof(struct arp_msg_wait_list));
    g_arp_wmsgs_h = NULL;
    g_arp_wmsgs_t = NULL;
    return MIP_OK;
}

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

static u32_t los_mip_get_wmsgs_head_tm(void)
{
    if(NULL != g_arp_wmsgs_h)
    {
        return g_arp_wmsgs_h->storetm;
    }
     return 0;   
}


int los_mip_tcpip_inpkt(struct netbuf *p, struct netif *inif) 
{
    int ret = 0;
    struct mip_msg *msg = NULL;

    ret = los_mip_mbox_valid(&g_core_msgbox);
    if(!ret)
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

int los_mip_tcpip_snd_msg(struct mip_msg *msg)
{
    if (los_mip_mbox_trypost(&g_core_msgbox, msg) != MIP_OK) 
    {
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    return MIP_OK;
}

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
int los_test_val(u32_t d)
{
    return d+1;
}
static void los_mip_tcpip_task(void *arg)
{
    u32_t ret = 0;
    int retfn = 0;
    uint32_t msgtm = 0;
    uint32_t curtm = 0;
    u32_t payload_len = 0;
    struct mip_msg *msgs = NULL;
    struct mip_msg *rsndmsgs = NULL;
    
    while(1)
    {
        curtm = osKernelSysTick();
        msgtm = los_mip_get_wmsgs_head_tm();
        if ( msgtm != 0
            && curtm - msgtm >= 20)
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
        switch(msgs->type)
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


void los_mip_tcpip_init(void *arg)
{
    los_mpools_init();
    los_mip_init_sockets();
    /* init tcp ip core message box */ 
    if(los_mip_mbox_new(&g_core_msgbox, MIP_TCPIP_MBOX_SIZE) != MIP_OK) 
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
