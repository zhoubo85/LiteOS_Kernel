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
#include "los_mip_netif.h"
#include "los_mip_ethernet.h"
#include "los_mip_ipv4.h"
#include "los_mip_tcp.h"


int los_mip_tcp_delte_seg(struct mip_tcp_seg *seg);
static int los_mip_tcp_send_synack(struct netif *dev, 
                                   struct mip_conn *con,
                                   struct tcp_hdr *tcph);
static inline int los_mip_tcp_enable_ctltmr(enum tcp_tmr_type type,
                                            struct mip_conn *con);
static inline int los_mip_tcp_disable_ctltmr(enum tcp_tmr_type type,
                                             struct mip_conn *con);
static inline int los_mip_tcp_get_lastseg_len(struct mip_conn *con);
static inline int los_mip_tcp_set_tmr_finwait2(struct mip_conn *con, 
                                               u16_t count);
static inline int los_mip_tcp_set_tmr_persit(struct mip_conn *con, 
                                               u16_t count);
static inline int los_mip_tcp_set_tmr_2msl(struct mip_conn *con, 
                                               u16_t count);
static inline int los_mip_tcp_set_tmr_rexmit(struct mip_conn *con, 
                                               u32_t rto);
int los_mip_tcp_check_send_now(struct mip_conn *con, int tmoutflag);
/*****************************************************************************
 Function    : los_mip_tcp_update_rto
 Description : update tcp rto value
                first rto calc
                1.  SRTT = R
                2.  RTTVAR = R/2
                3.  RTO = SRTT + max(rto_min, K*RTTVAR) , K = 4
                normal rto calc(use R' as cur rtt )
                RTTVAR = (1 - beta)*RTTVAR + beta*|SRTT - R'|
                SRTT = (1 - alpha)*SRTT + alpha*R' 
                RTO = SRTT + max(rto_min, K*RTTVAR)
                alpha = 1/8  beta = 1/4
 Input       : con @ connectionn's pointer
               currtt @ the last acked segment's rtt value 
 Output      : None
 Return      : tmp @ tcp segment pointer, if failed return NULL
 *****************************************************************************/
void los_mip_tcp_update_rto(struct mip_conn *con, u32_t currtt)
{
    //u32_t tmprtt = 0;
    u32_t tmprto = 0;
    u32_t tmprttvar = 0;
    u32_t tmpsrtt = 0;
    
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return ;
    }
    tmprttvar = con->ctlb.tcp->rttvar;
    tmprto = con->ctlb.tcp->rto;
    if (tmprttvar != 0)
    {
        /* not first rto calc */
        tmpsrtt = con->ctlb.tcp->srtt;
        tmprttvar = (3 * tmprttvar / 4) + ((tmpsrtt - currtt) / 4);
        tmpsrtt = (7 * tmpsrtt / 8) + (currtt / 8);
        tmprto = tmpsrtt + MIP_MAX(MIP_TCP_RTO_MIN, (tmprttvar * 4));
    }
    else
    {
        /* first rto calc */
        tmpsrtt = currtt;
        tmprttvar = currtt / 2;
        tmprto = tmpsrtt + MIP_MAX(MIP_TCP_RTO_MIN, (4 * tmprttvar)); 
    }
    con->ctlb.tcp->rttvar = tmprttvar;
    con->ctlb.tcp->rto = tmprto;
    con->ctlb.tcp->srtt = tmpsrtt;
}
/*****************************************************************************
 Function    : los_mip_tcp_new_seg
 Description : new a tcp segment 
 Input       : con @ connectionn's pointer
               opts @ option data for tcp
               optlen @ option data's length
               paylaod @ tcp payload data
               pldlen @ tcp payload data's length
               flags @ tcp header flags
               ack @ tcp header ack number value
               localip @ local ip address
 Output      : None
 Return      : tmp @ tcp segment pointer, if failed return NULL
 *****************************************************************************/
struct mip_tcp_seg *los_mip_tcp_new_seg(struct mip_conn *con, 
                                        u8_t *opts, u8_t optlen, 
                                        u8_t *payload, u16_t pldlen,
                                        u8_t flags, u32_t ack,
                                        ip_addr_t *localip)
{
    struct mip_tcp_seg *tmp = NULL;
    struct netbuf *p = NULL;
    struct tcp_hdr *tcph = NULL;
    u16_t len = 0;
    int ret = 0;
    
    if ((NULL == con) || (NULL == localip))
    {
        return NULL;
    }
    if ((NULL != opts) && (optlen != 0))
    {
        len += optlen;
    }
    if ((NULL != payload) && (pldlen != 0))
    {
        len += pldlen;
    }
    p = netbuf_alloc(NETBUF_TRANS, len);
    if (NULL == p)
    {
        return NULL;
    }
    tmp = (struct mip_tcp_seg *)los_mpools_malloc(MPOOL_TCP, 
                                                  sizeof(struct mip_tcp_seg));
    if (NULL == tmp)
    {
        netbuf_free(p);
        return NULL;
    }
    tmp->sndtm = los_mip_cur_tm();
    tmp->rearpcnt = 0;
    tmp->rexmitcnt = 0;
    memset(tmp, 0, sizeof(struct mip_tcp_seg));
    tmp->p = p;
    if (NULL != opts)
    {
         memcpy(p->payload, opts, optlen);
    }
    if (NULL != payload)
    {
        memcpy((u8_t *)p->payload + optlen, payload, pldlen);
    }
    ret = los_mip_jump_header(p, -MIP_TCP_HDR_LEN);
    if (ret != MIP_OK)
    {
        netbuf_free(p);
        los_mpools_free(MPOOL_TCP, tmp);
        return NULL;
    }
    memset(p->payload, 0, MIP_TCP_HDR_LEN);
    tcph = (struct tcp_hdr *)p->payload;
    tcph->ack = MIP_NTOHL(ack);
    tcph->seq = MIP_NTOHL(con->ctlb.tcp->snd_nxt);
    tcph->sport = con->ctlb.tcp->lport;
    tcph->dport = con->ctlb.tcp->rport;
    //flags = MIP_TCP_SYN | MIP_TCP_ACK;
    MIP_TCPH_HDRLEN_FLAGS_SET(tcph, 
                              ((MIP_TCP_HDR_LEN + optlen) / 4), 
                              flags);
    tcph->wndsize = MIP_HTONS(con->ctlb.tcp->swnd);
    tcph->chksum = 0;
    tcph->chksum = los_mip_checksum_pseudo_ipv4(p, MIP_PRT_TCP,
                                                p->len, localip,
                                                &con->ctlb.tcp->remote_ip);
    tmp->tcphdr = tcph;
    tmp->len = pldlen;
    return tmp;
}

static int los_mip_tcp_copy_unsnd(struct mip_conn *con, struct netbuf *p,
                                  int len)
{
    int tmpcpy = 0;
    char *data = NULL;
    struct netbuf *unsnd = NULL;
    struct netbuf *next = NULL;
    if ((NULL == con) || (NULL == p))
    {
        return -MIP_ERR_PARAM;
    }
    data = (char *)p->payload;
    unsnd = con->ctlb.tcp->unsend;

    while (NULL != unsnd)
    {
        if ((tmpcpy + unsnd->len) > len)
        {
            /* all the data we need has copied */
            memcpy(data + tmpcpy, unsnd->payload, len - tmpcpy);
            unsnd->payload = (char *)unsnd->payload + (len - tmpcpy);
            unsnd->len -= (len - tmpcpy);
            tmpcpy = len;
            break;
        }
        else
        {
            memcpy(data + tmpcpy, unsnd->payload, unsnd->len);
            tmpcpy += unsnd->len;
            next = unsnd->next;
            netbuf_free(unsnd);
            unsnd = NULL;
        }
        if (NULL == unsnd)
        {
            unsnd = next;
        }
    }
    /* todo: need some protect for unsndsize & unsend */
    con->ctlb.tcp->unsndsize -= len;
    con->ctlb.tcp->unsend = unsnd;

    return MIP_OK;
}
/*****************************************************************************
 Function    : los_mip_tcp_new_dataseg
 Description : new a tcp data segment 
 Input       : con @ connectionn's pointer
               opts @ option data for tcp
               optlen @ option data's length
               paylaod @ tcp payload data
               pldlen @ tcp payload data's length
               flags @ tcp header flags
               ack @ tcp header ack number value
               localip @ local ip address
 Output      : None
 Return      : tmp @ tcp segment pointer, if failed return NULL
 *****************************************************************************/
struct mip_tcp_seg *los_mip_tcp_new_dataseg(struct mip_conn *con,
                                        u8_t flags, u32_t ack,
                                        ip_addr_t *localip)
{
    struct mip_tcp_seg *tmp = NULL;
    struct netbuf *p = NULL;
    struct tcp_hdr *tcph = NULL;
    u16_t len = 0;
    int ret = 0;
    int mss = 0;
    int unsnd = 0;
    
    if ((NULL == con) || (NULL == localip))
    {
        return NULL;
    }
    /* fisrt should calc the send data's length */
    mss = (con->ctlb.tcp->mss > con->ctlb.tcp->localmss) ? con->ctlb.tcp->localmss : con->ctlb.tcp->mss;
    unsnd = con->ctlb.tcp->unsndsize;
    if (unsnd > mss)
    {
        /* more than one segment data */
        len = mss;
    }
    else
    {
        len = unsnd;
    }
    /* send data's length should not > remote's congestion window size */
    if (len > con->ctlb.tcp->cwnd)
    {
        len = con->ctlb.tcp->cwnd;

        /* start persist timer */
        los_mip_tcp_set_tmr_persit(con, MIP_TCP_PERSIST_TIMEOUT);
        los_mip_tcp_enable_ctltmr(TCP_TMR_P, con);
    }
    p = netbuf_alloc(NETBUF_TRANS, len);
    if (NULL == p)
    {
        return NULL;
    }
    /* copy send data to tcp segment */
    los_mip_tcp_copy_unsnd(con, p, len);
    tmp = (struct mip_tcp_seg *)los_mpools_malloc(MPOOL_TCP, 
                                                  sizeof(struct mip_tcp_seg));
    if (NULL == tmp)
    {
        netbuf_free(p);
        return NULL;
    }
    tmp->sndtm = los_mip_cur_tm();
    memset(tmp, 0, sizeof(struct mip_tcp_seg));
    tmp->p = p;

    ret = los_mip_jump_header(p, -MIP_TCP_HDR_LEN);
    if (ret != MIP_OK)
    {
        netbuf_free(p);
        los_mpools_free(MPOOL_TCP, tmp);
        return NULL;
    }
    memset(p->payload, 0, MIP_TCP_HDR_LEN);
    tcph = (struct tcp_hdr *)p->payload;
    tcph->ack = MIP_NTOHL(ack);
    tcph->seq = MIP_NTOHL(con->ctlb.tcp->snd_nxt);
    tcph->sport = con->ctlb.tcp->lport;
    tcph->dport = con->ctlb.tcp->rport;
    //flags = MIP_TCP_SYN | MIP_TCP_ACK;
    MIP_TCPH_HDRLEN_FLAGS_SET(tcph, 
                              ((MIP_TCP_HDR_LEN ) / 4), 
                              flags);
    tcph->wndsize = MIP_HTONS(con->ctlb.tcp->swnd);
    tcph->chksum = 0;
    tcph->chksum = los_mip_checksum_pseudo_ipv4(p, MIP_PRT_TCP,
                                                p->len, localip,
                                                &con->ctlb.tcp->remote_ip);
    tmp->tcphdr = tcph;
    tmp->len = len;
    return tmp;
}

/*****************************************************************************
 Function    : los_mip_tcp_create_recvseg
 Description : create a tcp segment that recived from remote 
 Input       : con @ connectionn's pointer
               p @ netbuf that recieved
               tcph @ tcp header info
               pldlen @ tcp payload data's length
 Output      : None
 Return      : tmp @ tcp segment pointer, if failed return NULL
 *****************************************************************************/
struct mip_tcp_seg *los_mip_tcp_create_recvseg(struct mip_conn *con, 
                                               struct netbuf *p, 
                                               struct tcp_hdr *tcph,
                                               int pldlen)
{
    struct mip_tcp_seg *tmp = NULL;

    
    if ((NULL == con) || (NULL == p))
    {
        return NULL;
    }
    tmp = (struct mip_tcp_seg *)los_mpools_malloc(MPOOL_TCP, 
                                                  sizeof(struct mip_tcp_seg));
    if (NULL == tmp)
    {
        //netbuf_free(p);
        return NULL;
    }
    memset(tmp, 0, sizeof(struct mip_tcp_seg));
    tmp->p = p;
    los_mip_jump_header(p, MIP_TCPH_HDRLEN(tcph)*4);
    tmp->tcphdr = tcph;
    tmp->len = pldlen;
    
    return tmp;
}

/*****************************************************************************
 Function    : los_mip_tcp_delte_seg
 Description : delete tcp segment data
 Input       : seg @ the segment's pointer which will be deleted
 Output      : None
 Return      : MIP_OK means free segment buffer finished.
 *****************************************************************************/
int los_mip_tcp_delte_seg(struct mip_tcp_seg *seg)
{
    if (NULL == seg)
    {
        return MIP_OK;
    }
    if (NULL != seg->p)
    {
        netbuf_free(seg->p);
        seg->p = NULL;
        los_mpools_free(MPOOL_TCP, seg);
        seg = NULL;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_add_seg_unacked
 Description : add a tcp segment to unacked list
 Input       : conn @ connectionn's pointer
               seg @ the tcp segment that not acked yet
 Output      : None
 Return      : MIP_OK @ add ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_add_seg_unacked(struct mip_conn *conn, 
                                struct mip_tcp_seg *seg)
{
    struct mip_tcp_seg *tmp = NULL;
    if ((NULL == conn) || (NULL == seg))
    {
        return -MIP_ERR_PARAM;
    }
    if (NULL == conn->ctlb.tcp->unacked)
    {
        conn->ctlb.tcp->unacked = seg;
    }
    else
    {
        seg->next = NULL;
        for (tmp = conn->ctlb.tcp->unacked; tmp->next != NULL; tmp = tmp->next);
        tmp->next = seg;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_add_seg_recieved
 Description : add a tcp segment to the tail of the recieved list
 Input       : conn @ connectionn's pointer
               seg @ the tcp segment that not acked yet
 Output      : None
 Return      : MIP_OK @ add ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_add_seg_recieved(struct mip_conn *conn, 
                                 struct mip_tcp_seg *seg)
{
    struct mip_tcp_seg *tmp = NULL;
    if ((NULL == conn) || (NULL == seg))
    {
        return -MIP_ERR_PARAM;
    }
    if (NULL == conn->ctlb.tcp->recievd)
    {
        conn->ctlb.tcp->recievd = seg;
    }
    else
    {
        seg->next = NULL;
        for (tmp = conn->ctlb.tcp->recievd; tmp->next != NULL; tmp = tmp->next);
        tmp->next = seg;
    }
    if(conn->state & STAT_RD)
    {
        los_mip_snd_msg_to_con(conn, (void *)conn->ctlb.tcp->recievd);
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_add_seg_unacked
 Description : delete one unacked segment in a tcp segment un-acked list
 Input       : conn @ connectionn's pointer
 Output      : None
 Return      : ret @ MIP_OK means process ok, other value means error happened
 *****************************************************************************/
int los_mip_tcp_remove_seg_unacked(struct mip_conn *conn)
{
    struct mip_tcp_seg *tmp = NULL;
    int ret = MIP_OK; 
    u32_t rtt = 0;
    if (NULL == conn)
    {
        return -MIP_ERR_PARAM;
    }
    if (NULL != conn->ctlb.tcp->unacked)
    {
        tmp = conn->ctlb.tcp->unacked->next;
        if (los_mip_cur_tm() > tmp->sndtm)
        {
            rtt = los_mip_cur_tm() - tmp->sndtm;
            /* call rtt update function */
        }
        else
        {
            rtt = los_mip_cur_tm() + 0xfffffffFU - tmp->sndtm;
        }
        /* update connection's rto */
        los_mip_tcp_update_rto(conn, rtt);
        
        ret = los_mip_tcp_delte_seg(conn->ctlb.tcp->unacked);
        conn->ctlb.tcp->unacked = tmp;
        
        /* tell core task to send data */
        los_mip_con_send_tcp_msg(conn, TCP_SENDI);
    }

    return ret;
}
                                            
/*****************************************************************************
 Function    : los_mip_tcp_remove_seg_mulunacked
 Description : delete unacked segment in a tcp segment un-acked list by the
               ack number.
 Input       : conn @ connectionn's pointer
 Output      : None
 Return      : ret @ MIP_OK means process ok, other value means error happened
 *****************************************************************************/
int los_mip_tcp_remove_seg_mulunacked(struct mip_conn *conn, u32_t ack)
{
    struct mip_tcp_seg *cur = NULL;
    struct mip_tcp_seg *next = NULL;
    int ret = MIP_OK; 
    u32_t rtt = 0;
    if (NULL == conn)
    {
        return -MIP_ERR_PARAM;
    }
    
    cur = conn->ctlb.tcp->unacked;
    while (NULL != cur)
    {
        next = cur->next;
        if (ack >= (MIP_NTOHL(cur->tcphdr->seq) + cur->len))
        {
            if (ack == (MIP_NTOHL(cur->tcphdr->seq) + cur->len))
            {
                if (los_mip_cur_tm() > cur->sndtm)
                {
                    rtt = los_mip_cur_tm() - cur->sndtm;
                    /* call rtt update function */
                }
                else
                {
                    rtt = los_mip_cur_tm() + 0xfffffffFU - cur->sndtm;
                }
                /* update connection's rto */
                los_mip_tcp_update_rto(conn, rtt);
            }
            ret = los_mip_tcp_delte_seg(cur);
            cur = NULL;
        }
        else
        {
            break;
        }
        cur = next;
        //conn->ctlb.tcp->unacked = tmp;
    }
    conn->ctlb.tcp->unacked = cur;
    /* tell core task to send data */
    los_mip_con_send_tcp_msg(conn, TCP_SENDI);
    
    return ret;
}


/*****************************************************************************
 Function    : los_mip_tcp_send_ack
 Description : make a ack tcp segment and send it out
 Input       : con @ listen connectionn's pointer
               dev @ net dev's handle which the data come from
 Output      : None
 Return      : ret @ MIP_OK process syn and send syn_ack ok, other failed
 *****************************************************************************/
static int los_mip_tcp_send_ack(struct netif *dev, struct mip_conn *con)
{
    int ret = 0;
    struct mip_tcp_seg *seg = NULL;
    u8_t flags = 0;
    
    if ((NULL == con) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    /* the connection is for the package , so we don't need jugde it */
    flags = MIP_TCP_ACK;
    seg = los_mip_tcp_new_seg(con, NULL, 0, NULL, 0, flags, 
                              con->ctlb.tcp->rcv_nxt,
                              &dev->ip_addr);
    if (NULL != seg)
    {
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        /* only ack message don't need add to un acked list */
        los_mip_tcp_delte_seg(seg);
        seg = NULL;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_send_zeroprobe
 Description : make a zero window probe tcp segment and send it out
 Input       : con @ listen connectionn's pointer
               dev @ net dev's handle which the data come from
 Output      : None
 Return      : ret @ MIP_OK process syn and send syn_ack ok, other failed
 *****************************************************************************/
static int los_mip_tcp_send_zeroprobe(struct netif *dev, struct mip_conn *con)
{
    int ret = 0;
    struct mip_tcp_seg *seg = NULL;
    u8_t flags = 0;
    u8_t data = 0x00;
    if ((NULL == con) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    flags = MIP_TCP_ACK;
    seg = los_mip_tcp_new_seg(con, NULL, 0, &data, 1, flags, 
                              con->ctlb.tcp->rcv_nxt,
                              &dev->ip_addr);
    if (NULL != seg)
    {
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        /* zero window probe segment, need add to retransmit list */
        los_mip_tcp_add_seg_unacked(con, seg);
        seg = NULL;
        los_mip_tcp_set_tmr_rexmit(con, con->ctlb.tcp->rto);
        los_mip_tcp_enable_ctltmr(TCP_TMR_REXMIT, con);
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_parse_opt
 Description : to deal with options, now just get mss option's value
 Input       : con @ listen connectionn's pointer
               tcph @ tcp package that we recived.
 Output      : None
 Return      : ret @ MIP_OK process option ok, other failed
 *****************************************************************************/
static int los_mip_tcp_parse_opt(struct mip_conn *con, struct tcp_hdr *tcph)
{
    int optlen = 0;
    int headlen = 0;
    char *optbase = NULL;
    int nextoffset = 0;
    u16_t mss = 0;
    if ((NULL == con) || (NULL == tcph))
    {
        return -MIP_ERR_PARAM;
    }
    headlen = MIP_TCPH_HDRLEN(tcph) * 4;
    optlen = headlen - MIP_TCP_HDR_LEN;
    optbase = (char *)tcph + MIP_TCP_HDR_LEN;

    if (optlen)
    {
        while(optlen > 0)
        {
            optlen -= nextoffset;
            optbase += nextoffset;
            switch(optbase[0])
            {
                case MIP_TCP_OPT_END:
                    /* option end, so we need stop process */
                    nextoffset = 1;
                    if (optlen)
                    {
                        return -MIP_ERR_PARAM;
                    }
                    break;
                case MIP_TCP_OPT_NON:
                    nextoffset = 1;
                    break;
                case MIP_TCP_OPT_MSS:
                    nextoffset = optbase[1];
                    if (MIP_TCP_OPT_MSS_LEN == nextoffset)
                    {
                        mss = (optbase[2] << 8);
                        mss = mss | optbase[3];
                        con->ctlb.tcp->mss = mss;
                    }
                    else
                    {
                        return -MIP_ERR_PARAM;
                    }
                    break;
                case MIP_TCP_OPT_SCA:
                    nextoffset = optbase[1];
                    break;  
                case MIP_TCP_OPT_UD4:
                case MIP_TCP_OPT_UD5:
                case MIP_TCP_OPT_UD6:
                case MIP_TCP_OPT_UD7:
                case MIP_TCP_OPT_TIM:
                    nextoffset = optbase[1];
                    break;                  
                default:
                    break;
            }
        }
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_listen_syn
 Description : to deal with syn message, and send sync ack message
 Input       : con @ listen connectionn's pointer
               tcph @ tcp package that we recived.
               src @ the ip address that which the package come from
 Output      : None
 Return      : ret @ MIP_OK process syn and send syn_ack ok, other failed
 *****************************************************************************/
static int los_mip_tcp_listen_syn(struct netif *dev, struct mip_conn *con, 
                                  struct tcp_hdr *tcph, ip_addr_t *src)
{
    int ret = 0;

    if ((NULL == con) || (NULL == tcph) || (NULL == src))
    {
        return -MIP_ERR_PARAM;
    }
    /* the connection is for the package , so we don't need jugde it */
    
    /* get syn message , need clone a new connection to deal with it */
    con->ctlb.tcp->state = TCP_SYN_RCVD;
    con->ctlb.tcp->rcv_nxt = MIP_NTOHL(tcph->seq) + 1;
    con->ctlb.tcp->snd_nxt = con->ctlb.tcp->isn;
    //con->ctlb.tcp->lastack = MIP_NTOHL(tcph->ack);
    con->ctlb.tcp->lastack = 0;
    con->ctlb.tcp->cwnd = MIP_NTOHS(tcph->wndsize);
    los_mip_tcp_parse_opt(con, tcph);
    ret = los_mip_tcp_send_synack(dev, con, tcph);
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_send_syn
 Description : make syn message and send it out
 Input       : dev @ network device that the syn message come from
               con @ listen connectionn's pointer
 Output      : None
 Return      : ret @ MIP_OK means send syn message ok other means error.
 *****************************************************************************/
int los_mip_tcp_send_syn(struct netif *dev, struct mip_conn *con)
{
    struct mip_tcp_seg *seg = NULL;
    u8_t optmss[4] = {0};/*option for mss */
    u8_t flags = 0;
    int ret = 0;
    
    optmss[0] = 0x02;
    optmss[1] = 0x04;
    optmss[2] = (con->ctlb.tcp->localmss & 0xff00) >> 8;
    optmss[3] = con->ctlb.tcp->localmss & 0x00ff;
    flags = MIP_TCP_SYN;
    
    seg = los_mip_tcp_new_seg(con, optmss, MIP_TCP_OPT_MSS_LEN, 
                              NULL, 0, flags, con->ctlb.tcp->rcv_nxt,
                              &dev->ip_addr);
    if (NULL != seg)
    {
        //con->ctlb.tcp->snd_nxt++;
        //dev->ip_addr.addr
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        if (ret == MIP_OK)
        {
            con->ctlb.tcp->snd_nxt++;
        }
        /* todo: can't delete here ,need store it wait ack */
        los_mip_tcp_set_tmr_rexmit(con, con->ctlb.tcp->rto);
        los_mip_tcp_add_seg_unacked(con, seg);
        seg = NULL;
        los_mip_tcp_enable_ctltmr(TCP_TMR_REXMIT, con);
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_send_synack
 Description : make syn_ack message and send it out
 Input       : dev @ network device that the syn message come from
               con @ listen connectionn's pointer
               tcph @ tcp package that we recived.
 Output      : None
 Return      : ret @ MIP_OK means send syn_ack message ok other means error.
 *****************************************************************************/
int los_mip_tcp_send_synack(struct netif *dev, struct mip_conn *con,
                                   struct tcp_hdr *tcph) 
{
    struct mip_tcp_seg *seg = NULL;
    u8_t optmss[4] = {0};/*option for mss */
    u8_t flags = 0;
    int ret = 0;
    
    optmss[0] = 0x02;
    optmss[1] = 0x04;
    optmss[2] = (con->ctlb.tcp->localmss & 0xff00) >> 8;
    optmss[3] = con->ctlb.tcp->localmss & 0x00ff;
    flags = MIP_TCP_SYN | MIP_TCP_ACK;
    
    seg = los_mip_tcp_new_seg(con, optmss, MIP_TCP_OPT_MSS_LEN, 
                              NULL, 0, flags, con->ctlb.tcp->rcv_nxt,
                              &dev->ip_addr);
    if (NULL != seg)
    {
        //con->ctlb.tcp->snd_nxt++;
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        if (MIP_OK == ret)
        {
            con->ctlb.tcp->snd_nxt++;
        }
        /* todo: can't delete here ,need store it wait ack */
        los_mip_tcp_set_tmr_rexmit(con, con->ctlb.tcp->rto);
        los_mip_tcp_add_seg_unacked(con, seg);
        los_mip_tcp_enable_ctltmr(TCP_TMR_REXMIT, con);
        seg = NULL;
    }

    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_send_paylaod
 Description : send tcp data to remote 
 Input       : con @ listen connectionn's pointer
               dev @ net dev's handle which the data come from
 Output      : None
 Return      : ret @ MIP_OK process syn and send reset ok, other failed
 *****************************************************************************/
int los_mip_tcp_send_paylaod(struct netif *dev, struct mip_conn *con)
{
    struct mip_tcp_seg *seg = NULL;
    u8_t flags = 0;
    int ret = 0;
    
    flags = MIP_TCP_PSH | MIP_TCP_ACK;
    
//    seg = los_mip_tcp_new_seg(con, NULL, 0, (u8_t *)payload, datalen, 
//                              flags, con->ctlb.tcp->rcv_nxt,
//                              &dev->ip_addr);
    seg = los_mip_tcp_new_dataseg(con, flags, con->ctlb.tcp->rcv_nxt,
                                  &dev->ip_addr);
    if (NULL != seg)
    {
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        if (MIP_OK == ret)
        {
            con->ctlb.tcp->snd_nxt += seg->len;
        }
        /* todo: can't delete here ,need store it wait ack */
        los_mip_tcp_set_tmr_rexmit(con, con->ctlb.tcp->rto);
        los_mip_tcp_add_seg_unacked(con, seg);
        if (con->ctlb.tcp->flag & TCP_NODELAY)
        {
            /* tell tcp stack to send data */
            los_mip_con_send_tcp_msg(con, TCP_SENDI);
        }
        seg = NULL;
    }

    return ret;
}
/*****************************************************************************
 Function    : los_mip_tcp_send_fin
 Description : make fin message and send it out
 Input       : dev @ network device that the syn message come from
               con @ listen connectionn's pointer
 Output      : None
 Return      : ret @ MIP_OK means send fin message ok other means error.
 *****************************************************************************/
int los_mip_tcp_send_fin(struct netif *dev, struct mip_conn *con)
{
    struct mip_tcp_seg *seg = NULL;
    u8_t flags = 0;
    int ret = 0;
    
    flags = MIP_TCP_FIN | MIP_TCP_ACK;
    seg = los_mip_tcp_new_seg(con, NULL, 0, NULL, 0, 
                              flags, con->ctlb.tcp->rcv_nxt,
                              &dev->ip_addr);
    if (NULL != seg)
    {
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        if (ret == MIP_OK)
        {
            con->ctlb.tcp->snd_nxt++;
        }
        /* todo: can't delete here ,need store it wait ack */
        los_mip_tcp_set_tmr_rexmit(con, con->ctlb.tcp->rto);
        los_mip_tcp_add_seg_unacked(con, seg);
        los_mip_tcp_enable_ctltmr(TCP_TMR_REXMIT, con);
        seg = NULL;
    }
    return ret;
}


/*****************************************************************************
 Function    : los_mip_tcp_send_rst
 Description : make a reset tcp segment and send it out
 Input       : con @ listen connectionn's pointer
               dev @ net dev's handle which the data come from
 Output      : None
 Return      : ret @ MIP_OK process syn and send reset ok, other failed
 *****************************************************************************/
static int los_mip_tcp_send_rst(struct netif *dev, struct mip_conn *con)
{
    int ret = 0;
    struct mip_tcp_seg *seg = NULL;
    u8_t flags = 0;
    
    if ((NULL == con) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    /* the connection is for the package , so we don't need jugde it */
    flags = MIP_TCP_RST;
    seg = los_mip_tcp_new_seg(con, NULL, 0, NULL, 0, flags, 
                              con->ctlb.tcp->rcv_nxt,
                              &dev->ip_addr);
    if (NULL != seg)
    {
        //con->ctlb.tcp->snd_nxt++;
        con->ctlb.tcp->snd_nxt = 0;
        ret = los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                                  &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        /* todo: reset message send, so we need close the connecttion */
        los_mip_tcp_delte_seg(seg);
        seg = NULL;
        /* set connection to closed stated */
        con->ctlb.tcp->state = TCP_CLOSED;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_send_rst
 Description : make a reset tcp segment and send it out
 Input       : p @ the tcp package that received 
               dev @ net dev's handle which the data come from
               src @ the ip address where the package come from.
 Output      : None
 Return      : ret @ MIP_OK process syn and send reset ok, other failed
 *****************************************************************************/
static int los_mip_tcp_send_rst2(struct netif *dev, struct netbuf *p,
                                 ip_addr_t *src)
{
    int ret = 0;
//    struct mip_tcp_seg *seg = NULL;
    u8_t flags = 0;
    struct tcp_hdr *tcph = NULL;
    ip_addr_t tmpsrc;
    u32_t tmp32 = 0;
    u16_t tmp16 = 0;
    if ((NULL == p) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    /* the connection is for the package , so we don't need jugde it */
    flags = MIP_TCP_RST | MIP_TCP_ACK;
    tcph = (struct tcp_hdr *)p->payload;
    tmp16 = tcph->sport;
    tcph->sport = tcph->dport;
    tcph->dport = tmp16;
    /* here maybe some bugs */
    tcph->seq = los_mip_tcp_gen_isn();
    tmp32 = MIP_NTOHL(tcph->seq) + 1;
    tcph->ack = MIP_NTOHL(tmp32);
    tcph->wndsize = MIP_NTOHS(LOS_MIP_TCP_WIND_SIZE);
    tcph->chksum = 0;
    MIP_TCPH_HDRLEN_FLAGS_SET(tcph, 
                              ((MIP_TCP_HDR_LEN) / 4), 
                              flags);
    tcph->chksum = los_mip_checksum_pseudo_ipv4(p, MIP_PRT_TCP,
                                                p->len, &dev->ip_addr,
                                                src);
    p->len = MIP_TCP_HDR_LEN;
    tmpsrc.addr = src->addr;
    ret = los_mip_ipv4_output(dev, p, &dev->ip_addr, 
                                  &tmpsrc, 255, 0, MIP_PRT_TCP);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_send_keepalive
 Description : send keepalive message to remote ip address.
 Input       : con @ listen connectionn's pointer
               dev @ net dev's handle which the data come from
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_tcp_send_keepalive(struct netif *dev, struct mip_conn *con)
{
    return los_mip_tcp_send_ack(dev, con);
}

/*****************************************************************************
 Function    : los_mip_tcp_process_synack
 Description : parse tcp input syn_ack package and send ack back.
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_synack(struct netif *dev, struct mip_conn *con, 
                                      struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = 0;
    flags = MIP_TCPH_FLAGS(tcph);

    if (MIP_TCP_FLAGS_SYN_ACK == flags)
    {
        //con->ctlb.tcp->rcv_nxt++;
        con->ctlb.tcp->rcv_nxt = MIP_NTOHL(tcph->seq) + 1;
        con->ctlb.tcp->cwnd = MIP_NTOHS(tcph->wndsize);
        los_mip_tcp_parse_opt(con, tcph); 
        /* three handle shake need send_next + 1 */
        //con->ctlb.tcp->snd_nxt++;
        ret = los_mip_tcp_send_ack(dev, con);
        if (ret == MIP_OK)
        {
            /* post message to upper layer tell it connect ok */
            con->ctlb.tcp->state = TCP_ESTABLISHED;
            /* remove syn seg from unacked list */
            los_mip_tcp_remove_seg_unacked(con);
        }
        /* send a message to connect function , connect always use block mode */
        los_mip_sem_signal(&con->op_finished);
        /* record the last acked seqence number */
        con->ctlb.tcp->lastack = MIP_NTOHL(tcph->ack) - 1;
    }
    else
    {
        /* Not the package we need, need discard it or send reset */
        //con->ctlb.tcp->state = TCP_CLOSED;
        los_mip_tcp_send_rst(dev, con);
        los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
        los_mip_tcp_remove_seg_unacked(con);
        
        /* tell upper layer connect failed, maybe upper layer need close socket */
        los_mip_sem_signal(&con->op_finished);
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_process_fin_wait1
 Description : parse tcp input fin_ack or ack package, 
               while tcp is in FIN_WAIT_1
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_fin_wait1(struct netif *dev, struct netbuf *p,
                                         struct mip_conn *con, 
                                         struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = 0;
    u32_t ack = 0;
    flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
    int payloadlen = 0;
    struct mip_tcp_seg *seg = NULL;
    int last_seq_len = 0;
//    if (ack != con->ctlb.tcp->snd_nxt)//&& (MIP_TCP_FLAGS_FIN_ACK != flags)
//    {
//        /* seq error, need send reset. */
//        los_mip_tcp_send_rst(dev, con);
//        los_mip_tcp_remove_seg_unacked(con);
//        los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
//        
//        netbuf_free(p);
//        p = NULL;
//        return MIP_OK;
//    }
    /* todo process input normal data package */
    last_seq_len = los_mip_tcp_get_lastseg_len(con);
    if (MIP_NTOHL(tcph->seq) < (con->ctlb.tcp->rcv_nxt - last_seq_len))
    {
        netbuf_free(p);
        p = NULL;
        /* seq error , need send rest*/
        los_mip_tcp_send_rst(dev, con);
        return -MIP_TCP_SEQ_NUM_ERR;
    }
    if (MIP_NTOHL(tcph->seq) == (con->ctlb.tcp->rcv_nxt - last_seq_len))
    {
        /* it's retransmit data, just send a ack */
        ret = los_mip_tcp_send_ack(dev, con);
        netbuf_free(p);
        p = NULL;
        return MIP_OK;
    }
    else if (flags & MIP_TCP_PSH)
    {
        /* todo : get data from remote , should store it and ack right now 
           we can call tcp in half-close state */
        payloadlen = p->len - MIP_TCPH_HDRLEN(tcph) * 4;
        if (payloadlen)
        {
            con->ctlb.tcp->lastseqlen = payloadlen;
            seg = los_mip_tcp_create_recvseg(con, p, tcph, payloadlen);
            if (NULL != seg)
            {
                los_mip_tcp_add_seg_recieved(con, seg);
                con->ctlb.tcp->rcv_nxt += payloadlen;
                /* update slide window size and send the ack */
                con->ctlb.tcp->swnd -= payloadlen;
                /* todo: need to check if there are data need send */
                ret = los_mip_tcp_send_ack(dev, con);
            }
        }
        /* fin's ack come with the data */
        if (ack == con->ctlb.tcp->snd_nxt)
        {
            /* tcp is TCP_FIN_WAIT_1 got ack, change to TCP_FIN_WAIT_2 */
            con->ctlb.tcp->state = TCP_FIN_WAIT_2;

            los_mip_tcp_remove_seg_unacked(con);
            los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
            /* start fin_wait2 timer. while the tiemr not timer out
               if data come from remote , should reset the timeout value
            */
            los_mip_tcp_set_tmr_finwait2(con, MIP_TCP_FIN_WAIT2_TIMEOUT);
            los_mip_tcp_enable_ctltmr(TCP_TMR_F, con);
        }
        else
        {
            
        }
        return MIP_OK;
    }
    switch(flags)
    {
        case MIP_TCP_FLAGS_ACK:
            if (ack == con->ctlb.tcp->snd_nxt)
            {
                /* tcp is TCP_FIN_WAIT_1 got ack, change to TCP_FIN_WAIT_2 */
                con->ctlb.tcp->state = TCP_FIN_WAIT_2;
                /* start fin_wait2 timer. while the tiemr not timer out
                   if data come from remote , should reset the timeout value
                */
                los_mip_tcp_enable_ctltmr(TCP_TMR_F, con);
            }
            else
            {
                /* ack >= snd_nxt seq error happend */
                los_mip_tcp_send_rst(dev, con);
                /* todo: need tell upper layer error close directly */
                //
            }
            break;
        case MIP_TCP_FLAGS_FIN_ACK:
            /* we send fin_a, remote give fin_b and ack_fin_a */
            if (ack == con->ctlb.tcp->snd_nxt)
            {
                /* send ack */
                ret = los_mip_tcp_send_ack(dev, con);
                /* change state to TCP_TIME_WAIT*/
                con->ctlb.tcp->state = TCP_TIME_WAIT;
                /* start timed wait state timer. while the tiemr not timer out
                   if data come from remote , should reset the timeout value
                   todo: need process rexmit package
                */
                los_mip_tcp_enable_ctltmr(TCP_TMR_2MSL, con);
                
            }
            else if (ack < con->ctlb.tcp->snd_nxt)
            {
                con->ctlb.tcp->rcv_nxt++;
                /* we send fin_a, remote give fin_b and ack < ack_fin_a 
                   this means client and server close the same time */
                ret = los_mip_tcp_send_ack(dev, con);
                /* todo: need process rexmit package change state to TCP_CLOSING */
                con->ctlb.tcp->state = TCP_CLOSING;
            }
            break;
        default:
            /* Not the package we need, need discard it or send reset */
            //con->ctlb.tcp->state = TCP_INITIAL;
            los_mip_tcp_send_rst(dev, con);
            /* todo: need tell upper layer error close directly */
            //los_mip_sem_signal(&con->op_finished);
            break;
    }
    if (seg == NULL)
    {
        netbuf_free(p);
        p = NULL;
    }
    /* get mesage so, need stop retransmit timer and remove unacked */
    los_mip_tcp_remove_seg_unacked(con);
    los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_process_fin_wait2
 Description : parse tcp input fin_ack or ack(with data) package, 
               while tcp is in FIN_WAIT_2
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_fin_wait2(struct netif *dev, struct netbuf *p, 
                                         struct mip_conn *con, 
                                         struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = 0;
    u32_t ack = 0;
    int payloadlen = 0;
    int last_seq_len = 0;
    struct mip_tcp_seg *seg = NULL;
    flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
    if (ack < con->ctlb.tcp->snd_nxt)//&& (MIP_TCP_FLAGS_FIN_ACK != flags)
    {
        netbuf_free(p);
        p = NULL;
        /* data err , need send reset*/
        los_mip_tcp_send_rst(dev, con);
        /* tell upper layer closed */
        return MIP_OK;
    }
    last_seq_len = los_mip_tcp_get_lastseg_len(con);
    if (MIP_NTOHL(tcph->seq) < (con->ctlb.tcp->rcv_nxt - last_seq_len))
    {
        netbuf_free(p);
        p = NULL;
        /* seq error , need send rest*/
        los_mip_tcp_send_rst(dev, con);
        return -MIP_TCP_SEQ_NUM_ERR;
    }
    if (MIP_NTOHL(tcph->seq) == (con->ctlb.tcp->rcv_nxt - last_seq_len))
    {
        /* it's retransmit data, just send a ack */
        ret = los_mip_tcp_send_ack(dev, con);
        netbuf_free(p);
        p = NULL;
        return ret;
    }
    else if (flags & MIP_TCP_PSH)
    {
        /* get data from remote , should store it and ack right now 
           we can call tcp in half-close state */
        payloadlen = p->len - MIP_TCPH_HDRLEN(tcph) * 4;
        if (payloadlen)
        {
            con->ctlb.tcp->lastseqlen = payloadlen;
            seg = los_mip_tcp_create_recvseg(con, p, tcph, payloadlen);
            if (NULL != seg)
            {
                los_mip_tcp_add_seg_recieved(con, seg);
                con->ctlb.tcp->rcv_nxt += payloadlen;
                /* update slide window size and send the ack */
                con->ctlb.tcp->swnd -= payloadlen;
                ret = los_mip_tcp_send_ack(dev, con);
                /* data received, need reset the fin_wait_2 timer out */
                los_mip_tcp_set_tmr_finwait2(con, MIP_TCP_FIN_WAIT2_TIMEOUT);
            }
        }
        /* todo: need recount the fin_wait_2's timeout timer */
        
        return ret;
    }
    switch(flags)
    {
        case MIP_TCP_FLAGS_FIN_ACK:
            if (ack == con->ctlb.tcp->snd_nxt)
            {
                con->ctlb.tcp->rcv_nxt = MIP_NTOHL(tcph->seq) + 1;
                ret = los_mip_tcp_send_ack(dev, con);
                con->ctlb.tcp->state = TCP_TIME_WAIT;
                los_mip_tcp_set_tmr_2msl(con, MIP_TCP_2MSL_TIMEOUT);
                los_mip_tcp_enable_ctltmr(TCP_TMR_2MSL, con);
            }
            else
            {
                /* data err , need send reset*/
                los_mip_tcp_send_rst(dev, con);
                /* todo: tell upper layer closed */
            }
            los_mip_tcp_disable_ctltmr(TCP_TMR_F, con);
            break;
        default:
            /* maybe data from remote server , and should restart TCP_TMR_F timer */
            
            break;
    }
    if (NULL == seg)
    {
        netbuf_free(p);
        p = NULL;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_process_closing
 Description : parse tcp input fin_ack or ack(with data) package, 
               while tcp is in CLOSING state
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_closing(struct netif *dev, 
                                       struct mip_conn *con, 
                                       struct tcp_hdr *tcph)
{
    //u8_t flags = 0;
    int ret = 0;
    u32_t ack = 0;
    //flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
    if (MIP_NTOHL(tcph->seq) == (con->ctlb.tcp->rcv_nxt - 1)
        && (MIP_TCP_FLAGS_FIN_ACK == ack))
    {
        /* retransmit fin_ack , only send ack message */
        ret = los_mip_tcp_send_ack(dev, con);
    }
    else if ((MIP_TCP_FLAGS_ACK == ack) 
        && (ack == con->ctlb.tcp->snd_nxt))
    {
        con->ctlb.tcp->state = TCP_TIME_WAIT;
        /* start 2MSL timer */
        los_mip_tcp_set_tmr_2msl(con, MIP_TCP_2MSL_TIMEOUT);
        los_mip_tcp_enable_ctltmr(TCP_TMR_2MSL, con);
    }
    else
    {
        /* error happend , send reset */
        ret = los_mip_tcp_send_rst(dev, con);
        /* todo: tell upper layer socket closed*/
    }

    return ret;
}
/*****************************************************************************
 Function    : los_mip_tcp_get_lastseg_len
 Description : get the last tcp segment's data length
 Input       : con @ socket connection's pointer
 Output      : None
 Return      : last_seq_len @ data length .
 *****************************************************************************/
static inline int los_mip_tcp_get_lastseg_len(struct mip_conn *con)
{
    int last_seq_len = 0;
    //struct mip_tcp_seg *seg = NULL;
    if (NULL == con)
    {
        return last_seq_len;
    }
    if (NULL != con->ctlb.tcp->recievd)
    {
        /* tcp package data length*/
//        seg = con->ctlb.tcp->recievd;
//        while(seg->next != NULL)
//        {
//            seg = seg->next;
//        }
        last_seq_len = con->ctlb.tcp->lastseqlen;
    }
    else
    {
        /* just syn or fin package virtual length*/
        last_seq_len = 1;
    }
    return last_seq_len;
}

/*****************************************************************************
 Function    : los_mip_tcp_store_data
 Description : process the data that recieved from remote .
 Input       : con @ socket connection's pointer
 Output      : None
 Return      : last_seq_len @ data length .
 *****************************************************************************/
static inline int los_mip_tcp_store_data(struct netif *dev, 
                                         struct mip_conn *con, 
                                         struct netbuf *p,
                                         struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    u8_t freeflag = 1;
    int ret = 0;
    u32_t ack = 0;
    struct mip_tcp_seg *seg = NULL;
    int last_seq_len = 0;
    int payloadlen = 0;
    
    flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);

    /* todo process input normal data package */
    last_seq_len = los_mip_tcp_get_lastseg_len(con);
    if (MIP_NTOHL(tcph->seq) < (con->ctlb.tcp->rcv_nxt - last_seq_len))
    {
        /* seq error , need send rest*/
        los_mip_tcp_send_rst(dev, con);
        netbuf_free(p);
        p = NULL;
        return -MIP_TCP_SEQ_NUM_ERR;
    }
    if (MIP_NTOHL(tcph->seq) == (con->ctlb.tcp->rcv_nxt - last_seq_len))
    {
        /* it's retransmit data, just send a ack */
        ret = los_mip_tcp_send_ack(dev, con);
        //return -MIP_TCP_SEQ_NUM_ERR;
    }
    else if (MIP_NTOHL(tcph->seq) == con->ctlb.tcp->rcv_nxt)
    {
        if (con->ctlb.tcp->swnd == 0)
        {
            /* slide window is empty , not need to revive this package */
            freeflag = 1;
            ret = los_mip_tcp_send_ack(dev, con);
        }
        else
        {
            if ((!con->ctlb.tcp->cwnd) && MIP_NTOHS(tcph->wndsize))
            {
                /* remote congistion window not full , can send data */
                los_mip_tcp_disable_ctltmr(TCP_TMR_P, con);
            }
            con->ctlb.tcp->cwnd = MIP_NTOHS(tcph->wndsize);
            if((flags == MIP_TCP_FLAGS_PSH_ACK)
                || (flags == MIP_TCP_FLAGS_ACK))
            {
                /* judge if unacked list need process */
                if (ack == con->ctlb.tcp->snd_nxt)
                {
                    /* remove unacked segment */
                    //los_mip_tcp_remove_seg_unacked(con);
                    los_mip_tcp_remove_seg_mulunacked(con, ack);
                }
                /* add data to tcp recieved list */
                payloadlen = p->len - MIP_TCPH_HDRLEN(tcph) * 4;
                if (payloadlen)
                {
                    seg = los_mip_tcp_create_recvseg(con, p, tcph, payloadlen);
                    if (NULL != seg)
                    {
                        freeflag = 0;
                        con->ctlb.tcp->lastseqlen = payloadlen;
                        los_mip_tcp_add_seg_recieved(con, seg);
                        con->ctlb.tcp->rcv_nxt += payloadlen;
                        /* update slide window size and send the ack */
                        con->ctlb.tcp->swnd -= payloadlen;
                        /* todo: need to check if there are data need send */
                        if(MIP_FALSE == los_mip_tcp_check_send_now(con, 0))
                        {
                            ret = los_mip_tcp_send_ack(dev, con);
                        }
                        
                    }
                }
            }
        }
        
    }
    else
    {
        /*
           seq > rcv_nxt , maybe later package come first or error  
           todo: if support sack , we need add it to mis-ordered list 
           if not support sack , we send reset, now we just reset it.
        */
        ret = los_mip_tcp_send_rst(dev, con);
    }
    if (freeflag)
    {
        netbuf_free(p);
        p = NULL;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_process_establish
 Description : parse tcp input data from remote , while tcp 
               is in ESTABLISHED state
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : ret @ MIP_OK means ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_establish(struct netif *dev, 
                                         struct netbuf *p, 
                                         struct mip_conn *con, 
                                         struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = 0;
    u32_t ack = 0;
    flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
//    int last_seq_len = 0;
    u8_t freeflag = 1;
    if (tcph->wndsize)
    {
        /* start persit timer */
    }
    if ((MIP_TCP_FLAGS_SYN_ACK == flags)
        && (con->ctlb.tcp->rcv_nxt == MIP_NTOHL(tcph->seq) + 1))
    {
        /*retransmit syn_ack, we need send ack */
        ret = los_mip_tcp_send_ack(dev, con);
        //return MIP_OK;
    }
    else if ((MIP_TCP_FLAGS_FIN_ACK == flags)
        && (ack == con->ctlb.tcp->snd_nxt))
    {
        con->ctlb.tcp->rcv_nxt++;
        con->ctlb.tcp->cwnd = MIP_NTOHS(tcph->wndsize);
        ret = los_mip_tcp_send_ack(dev, con);
        /* got fin_ack data ,need change to close wait state 
           while timer timeout happended, or all data send out
           need send fin message 
        */
        con->ctlb.tcp->state = TCP_CLOSE_WAIT;
        /* todo : tell upper layer no data can be read */
    }
    else
    {
        /* todo: process input normal data package */
        freeflag = 0;
        ret = los_mip_tcp_store_data(dev, con, p, tcph);
    }
    if (freeflag)
    {
        netbuf_free(p);
        p = NULL;
    }
    return ret;
}
/*****************************************************************************
 Function    : los_mip_tcp_recv_onlyack
 Description : called by cose_wait and last ack function, if not only ack 
               package come , send reset and change to closed state
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static inline int los_mip_tcp_recv_onlyack(struct netif *dev, 
                                           struct mip_conn *con, 
                                           struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = 0;
    u32_t ack = 0;
    flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
    
    if ((MIP_TCP_FLAGS_ACK != flags)
        || (ack != con->ctlb.tcp->snd_nxt))
    {
        /* Not correct data  */
        ret = los_mip_tcp_send_rst(dev, con);
        con->ctlb.tcp->state = TCP_CLOSED;
    }
    else
    {
        con->ctlb.tcp->cwnd = MIP_NTOHS(tcph->wndsize);
        /* if some data send , we need remove the unacked list */
        ret = los_mip_tcp_remove_seg_unacked(con);
    }
    los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
    
    return ret;
}
/*****************************************************************************
 Function    : los_mip_tcp_process_lastack
 Description : parse tcp input data from remote , while tcp 
               is in last_ack state
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_closewait(struct netif *dev, 
                                         struct mip_conn *con, 
                                         struct tcp_hdr *tcph)
{
    //u8_t flags = 0;
    //int ret = 0;
    u32_t ack = 0;
    //flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
    
    if (MIP_NTOHL(tcph->seq) == (con->ctlb.tcp->rcv_nxt - 1)
        && (MIP_TCP_FLAGS_FIN_ACK == ack))
    {
        /* retransmit fin_ack , only send ack message */
        los_mip_tcp_send_ack(dev, con);
    }
    return los_mip_tcp_recv_onlyack(dev, con, tcph);
}

/*****************************************************************************
 Function    : los_mip_tcp_process_lastack
 Description : parse tcp input data from remote , while tcp 
               is in last_ack state
 Input       : dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_process_lastack(struct netif *dev, 
                                       struct mip_conn *con, 
                                       struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = 0;
    u32_t ack = 0;
    flags = MIP_TCPH_FLAGS(tcph);
    ack = MIP_NTOHL(tcph->ack);
    
    if ((MIP_TCP_FLAGS_ACK == flags)
         && (ack == con->ctlb.tcp->snd_nxt))
    {
        con->ctlb.tcp->state = TCP_CLOSED;
    }
    else
    {
        /* Not correct data  */
        ret = los_mip_tcp_send_rst(dev, con);
    }
    los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
    los_mip_tcp_remove_seg_unacked(con);
    return ret;
}
/*****************************************************************************
 Function    : los_mip_tcp_process_package
 Description : parse tcp input package and send payload to upper layer
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
static int los_mip_tcp_establish(struct netif *dev, struct mip_conn *con, 
                                 struct netbuf *p, struct tcp_hdr *tcph)
{
    u8_t flags = 0;
    int ret = MIP_OK;
    
    struct skt_acpt_msg *scptmsg = NULL;
    flags = MIP_TCPH_FLAGS(tcph);
    if (MIP_TCP_FLAGS_ACK == flags)
    {
        /* send a message to listen socket tell upper layer */
        #if 1
        con->ctlb.tcp->state = TCP_ESTABLISHED;
        if(NULL != con->listen)
        {
            if (con->listen->state & STAT_ACCEPT)
            {
                scptmsg = los_mip_new_acptmsg(con);
                ret = los_mip_snd_acptmsg_to_listener(con->listen, scptmsg);
                if (ret != MIP_OK)
                {
                    /* send to accept box failed, so treat it as unaccept socket */
                    con->listen->backlog++;
                }
            }
            else
            {
                con->listen->backlog++;
            }
        }
        #else
        scptmsg = los_mip_new_acptmsg(con);
        ret = los_mip_snd_acptmsg_to_listener(con->listen, scptmsg);
        if (ret == MIP_OK)
        {
            con->ctlb.tcp->state = TCP_ESTABLISHED;
        }
        else
        {
            /* todo: failed add to listener */
            los_mip_tcp_send_rst(dev, con);
        }
        #endif
        /* record the last acked seqence number */
        con->ctlb.tcp->lastack = MIP_NTOHL(tcph->ack) - 1;
    }
    else
    {
        /* Not the package we need, need discard it or send reset */
        los_mip_tcp_send_rst(dev, con);
        /* todo: set the tcp connection closed */
    }
    /* recieved message so should stop retransmit timer*/
    los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
    /* remove first unacked buf */
    los_mip_tcp_remove_seg_unacked(con);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_process_package
 Description : parse tcp input package and send payload to upper layer
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               con @ socket connection's pointer
               tcph @ tcp header's pointer.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_tcp_process_package(struct netif *dev, struct mip_conn *con, 
                                struct netbuf *p, struct tcp_hdr *tcph)
{ 
    int state = 0;
    int freeflag = 1;
    if ((NULL == dev) || (NULL == con) || (NULL == tcph))
    {
        return -MIP_ERR_PARAM;
    }

    state = con->ctlb.tcp->state;
    switch (state)
    {
        case TCP_SYN_SENT:
            /* only deal with syn_ack message */
            los_mip_tcp_process_synack(dev, con, tcph);
            break;
        case TCP_SYN_RCVD:
            if (tcph->seq != MIP_NTOHL(con->ctlb.tcp->rcv_nxt))
            {
                /* todo: error seqno, need discard it or send reset */
                return -MIP_TCP_SEQ_NUM_ERR;
            }
            /* only deal with ack message */
            los_mip_tcp_establish(dev, con, p, tcph);
            break;
        case TCP_ESTABLISHED:
            /* connection already establisted, normal send/revice message */
            
            /* if got fin message ,send ack first , it's passive shutdown
               we enter close_wait state. upper layer read = 0, need close
               and then we need judge if the upper layer has call shutdown 
               1: called shutdown wr, after all data out send fin to remote
                  we enter last_ack state, ater that if we 
                  got ack enter closed. or never got ack just retransmit and 
                  finally tiemed out enter closed.
            */
            /* the psh_ack data need store, so the netbuf should not free here */
            freeflag = 0;
            los_mip_tcp_process_establish(dev, p, con, tcph);
            break;
        case TCP_CLOSE_WAIT:
            /* while all data send out need send fin , and only ack come is ok*/
            los_mip_tcp_process_closewait(dev, con, tcph);
            break;
        case TCP_LAST_ACK:
            los_mip_tcp_process_lastack(dev, con, tcph);
            break;
        case TCP_FIN_WAIT_1:
            /*  */
            freeflag = 0;
            los_mip_tcp_process_fin_wait1(dev, p, con, tcph);
            break;
        case TCP_FIN_WAIT_2:
            /* need wait remote give fin message , and then
               we can change to TIME_WAIT state 
               if long time not recived fin message, need use
               timer to do reset directly.
            */
            freeflag = 0;
            los_mip_tcp_process_fin_wait2(dev, p, con, tcph);
            break;
        case TCP_CLOSING:
            los_mip_tcp_process_closing(dev, con, tcph);
            break;
        case TCP_TIME_WAIT:
            /* got message , some error happed, in this state 
                after 2MSL ms , the socket go to closed state.
            */
            /* retransmit fin_ack from remote , so give ack */
            if (MIP_NTOHL(tcph->seq) == (con->ctlb.tcp->rcv_nxt - 1))
            {
                /* retransmit data , only send ack message */
                los_mip_tcp_send_ack(dev, con);
            }
            else
            {
                /* data err , need send reset*/
                los_mip_tcp_send_rst(dev, con);
            }
            /* todo: tell upper layer closed */
        default :
            /* no tcp state to deal with this message, send reset */
            los_mip_tcp_send_rst(dev, con);
            break;
    }
    if (freeflag)
    {
        netbuf_free(p);
        p = NULL;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_accept_list_full
 Description : judge if the listener's un-accepted list is full
 Input       : listencon @ listen connection's pointer
 Output      : None
 Return      : MIP_TRUE means list is full, other means not full.
 *****************************************************************************/
int los_mip_tcp_accept_list_full(struct mip_conn *listencon)
{
    u8_t backlog = 0;
    u8_t unaccept = 0;
    if (NULL == listencon)
    {
        return MIP_TRUE;
    }
    backlog = ((listencon->backlog & 0xff00) >> 8);
    unaccept = listencon->backlog & 0x00ff;
    if (unaccept < backlog)
    {
        return MIP_FALSE;
    }
    return MIP_TRUE;
}

/*****************************************************************************
 Function    : los_mip_tcp_input
 Description : parse tcp input package and send payload to upper layer
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               src @ source ip address of the package
               dst @ dest ip address of the package.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_tcp_input(struct netbuf *p, struct netif *dev, 
                      ip_addr_t *src, ip_addr_t *dst)
{
    struct mip_conn *tmpcon = NULL;
    struct mip_conn *newcon = NULL;
    struct tcp_hdr *tcph = NULL;

    u32_t chksum = 0x0;
    int ret = MIP_OK;
    u8_t hdrlen = 0;
   
    if ((NULL == p) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }

    tcph = (struct tcp_hdr *)p->payload;
    hdrlen = MIP_TCPH_HDRLEN(tcph) * 4;
    if ((p->len < MIP_TCP_HDR_LEN) || (p->len < hdrlen))
    {
        /* package or header length error */
        netbuf_free(p);
        return -MIP_TCP_PKG_LEN_ERR;
    }
    if ((u16_t)TYPE_IPV4 == los_mip_get_ip_ver(p))
    {
        chksum = los_mip_checksum_pseudo_ipv4(p, 
                                         (u8_t)MIP_PRT_TCP, 
                                         p->len, src, dst);
        if (chksum != 0)
        {
            /* checksum error */
            netbuf_free(p);
            p = NULL;
            return -MIP_TCP_PKG_CHKSUM_ERR;
        }
        tmpcon = los_mip_tcp_get_conn(p, src, dst);
        if (NULL != tmpcon)
        {
            /* find a conn to deal with the message */
            if (tmpcon->state & STAT_LISTEN)
            {
                if (los_mip_tcp_accept_list_full(tmpcon) == MIP_FALSE)
                {
                    newcon = los_mip_clone_conn(tmpcon, *dst, *src, tcph->sport);
                    /* syn come, give message to listen connection */
                    ret = los_mip_tcp_listen_syn(dev, newcon, tcph, src); 
                    if (ret != MIP_OK)
                    {
                        /* some error happend */
                    }
                }
                netbuf_free(p);
                p = NULL;
            }
            else
            {
                /* give message to normal tcp connection */
                ret = los_mip_tcp_process_package(dev, tmpcon, p, tcph);
            }
        }
        else
        {
            /* no connection is created for listen, discard data */
            //los_mip_tcp_send_rst2(dev, p, src);
            netbuf_free(p);
            p = NULL;
        }
    }
    /* todo: maybe ipv6 could be processed here, now not supported yet */

    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_check_send_now
 Description : check if can send tcp segment right now.
 Input       : conn @ tcp connection's pointer
               flag @ timeout flag , if 1 means this is called by internal
               timer's timeout callback function
 Output      : None
 Return      : MIP_TRUE means can send now,  MIP_FALSE means don't send now
               need wait timeout to send data.
 *****************************************************************************/
int los_mip_tcp_check_send_now(struct mip_conn *con, int tmoutflag)
{
    u8_t flag = 0;
    int datalen = 0;
    struct netbuf *tmp = NULL;
    int tmpmss = 0;
    int ret = MIP_TRUE;
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return MIP_FALSE;
    }
    
    if ((NULL == con->ctlb.tcp->unsend)
        || !con->ctlb.tcp->cwnd)
    {
        /* no data or remote no buffer, so can't send */
        return MIP_FALSE;
    }
    
    flag = con->ctlb.tcp->flag;
    if (flag & TCP_NODELAY)
    {
        return MIP_TRUE;
    }
    else
    {
        tmp = con->ctlb.tcp->unsend;
        tmpmss = con->ctlb.tcp->mss > con->ctlb.tcp->localmss ?\
                    con->ctlb.tcp->localmss:con->ctlb.tcp->mss;
        if (NULL != tmp)
        {
            if (con->ctlb.tcp->unacked != NULL)
            {
                ret = MIP_FALSE;
            }
            else
            {
                while(NULL != tmp)
                {
                    datalen += tmp->len;
                    if (datalen >= tmpmss)
                    {
                        ret = MIP_TRUE;
                        break;
                    }
                    tmp = tmp->next;
                }
                if ((datalen < tmpmss) && !tmoutflag)
                {
                    /* todo: need get timeout flag */
                    ret = MIP_FALSE;
                }
            }
        }
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_process_upper_msg
 Description : process the message that reviced from socket.
 Input       : conn @ tcp connection's pointer
               type @ the tcp message type
 Output      : None
 Return      : ret @ 0 means send ok, other value means error happened.
 *****************************************************************************/
int los_mip_tcp_process_upper_msg(void *msg)
{
    int ret = 0;
    struct skt_tcp_msg *tcpmsg = NULL;
    struct skt_acpt_msg *acptmsg = NULL;
    struct mip_conn * unacpt = NULL;
    struct netif *dev = NULL;
    int msgtype = 0;
    
    if (NULL == msg)
    {
        return MIP_OK;
    }
    tcpmsg = (struct skt_tcp_msg *)msg;
    dev = los_mip_ip_route(&tcpmsg->con->ctlb.tcp->remote_ip);
    msgtype = tcpmsg->type;
    switch(msgtype)
    {
        /* start to connect remote server */
        case TCP_CONN:
            /* socket connect remote server so, send syn message */
            ret = los_mip_tcp_send_syn(dev, tcpmsg->con);
            if (ret == MIP_OK)
            {
                tcpmsg->con->ctlb.tcp->state = TCP_SYN_SENT;
            }
            else
            {
                /* send syn message failed, tell upper layer */
                los_mip_sem_signal(&tcpmsg->con->op_finished);
            }
            break;
        /* user close socket */
        case TCP_CLOSE:
            /* socket connect remote server so, send syn message
               only when the tcp connection's state is TCP_ESTABLISHED */
            ret = los_mip_tcp_send_fin(dev, tcpmsg->con);
            if (ret == MIP_OK)
            {
                /* TCP_ESTABLISHED --> TCP_FIN_WAIT_1 */
                tcpmsg->con->ctlb.tcp->state = TCP_FIN_WAIT_1;
            }
            else
            {
                /*
                   send fin message failed, tell upper layer 
                   todo: need set internal error flag 
                */
                los_mip_sem_signal(&tcpmsg->con->op_finished);
            }
            break;
        case TCP_SENDD:
            /* 
               internal timer timeout, and datalen < mss , 
               send data immediately 
            */
            los_mip_tcp_send_paylaod(dev, tcpmsg->con);
            break;
        case TCP_SENDI:
            /* send data immediately */
            if (MIP_TRUE == los_mip_tcp_check_send_now(tcpmsg->con, 0))
            {
                /* send data immediately */
                los_mip_tcp_send_paylaod(dev, tcpmsg->con);
            }
            break;
        case TCP_READ:
            /* read data immediately */
            if (NULL != tcpmsg->con->ctlb.tcp->recievd)
            {
                /* send msg tell socket data buffer */
                los_mip_snd_msg_to_con(tcpmsg->con, 
                                       (void *)tcpmsg->con->ctlb.tcp->recievd);
            }
            break;
        case TCP_ACCEPT:
            if (tcpmsg->con->backlog & 0x00ff)
            {
                /* have un-accepted connection, need send to acceptbox */
                /* todo: find the conn */
                unacpt = los_mip_tcp_get_unaccept_conn(tcpmsg->con);
                acptmsg = los_mip_new_acptmsg(unacpt);
                if (NULL != unacpt)
                {
                    los_mip_snd_acptmsg_to_listener(tcpmsg->con, acptmsg);
                    tcpmsg->con->backlog--;
                }
            }
            break;
        default:
            break;
    }
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_gen_isn
 Description : generate the random tcp isn value.
 Input       : None
 Output      : None
 Return      : u32_t @ 32 bit unsigned random data.
 *****************************************************************************/
u32_t los_mip_tcp_gen_isn(void)
{
    return los_mip_random();
}

/*****************************************************************************
 Function    : los_mip_tcp_enable_ctltmr
 Description : enable a tcp timer that based on internal 200ms timer.
 Input       : type @ timer's type 
 Output      : None
 Return      : MIP_OK @ means eable ok, other mean faild.
 *****************************************************************************/
static inline int los_mip_tcp_enable_ctltmr(enum tcp_tmr_type type, 
                                            struct mip_conn *con)
{
    int tmptype = type;
    if ((type >= TCP_TMR_MAX) || (type < TCP_TMR_K) 
        || (NULL == con) || (NULL == con->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    con->ctlb.tcp->tmrmask |= (0x01 << tmptype);

    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_set_tmr_finwait2
 Description : set fin_wait_2 timer's count value.
 Input       : conn @ tcp connection's pointer
               count @ the count number
 Output      : None
 Return      : MIP_OK @ set ok, other mean faild.
 *****************************************************************************/
static inline int los_mip_tcp_set_tmr_finwait2(struct mip_conn *con, 
                                               u16_t count)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    /* default is MIP_TCP_FIN_WAIT2_TIMEOUT */
    con->ctlb.tcp->finwait2 = count;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_set_tmr_finwait2
 Description : set persist timer's count value.
 Input       : conn @ tcp connection's pointer
               count @ the count number
 Output      : None
 Return      : MIP_OK @ set ok, other mean faild.
 *****************************************************************************/
static inline int los_mip_tcp_set_tmr_persit(struct mip_conn *con, 
                                               u16_t count)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    /* default is MIP_TCP_PERSIST_TIMEOUT */
    if (!con->ctlb.tcp->persist)
    {
        con->ctlb.tcp->persist = count;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_set_tmr_2msl
 Description : set 2MSL timer's count value.
 Input       : conn @ tcp connection's pointer
               count @ the count number
 Output      : None
 Return      : MIP_OK @ set ok, other mean faild.
 *****************************************************************************/
static inline int los_mip_tcp_set_tmr_2msl(struct mip_conn *con, 
                                               u16_t count)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    /* default is MIP_TCP_2MSL_TIMEOUT */
    con->ctlb.tcp->twomsl = count;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_backoff_rto
 Description : get backed-off rto value.
 Input       : cnt @ the retransmit count
               rto @ current rto value, default is 200 ms
 Output      : None
 Return      : MIP_OK @ set ok, other mean faild.
 *****************************************************************************/
static inline u32_t los_mip_tcp_backoff_rto(u32_t cnt, u32_t rto)
{
    int i = 0;
    u32_t ret = 1;
    for(i = 0; i < cnt; i++)
    {
        ret = ret * 2;
    }
    ret *= rto;
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_set_tmr_rexmit
 Description : set retransmit timer's count value.
 Input       : conn @ tcp connection's pointer
               rto @ current rto value, default is 200 ms
 Output      : None
 Return      : MIP_OK @ set ok, other mean faild.
 *****************************************************************************/
static inline int los_mip_tcp_set_tmr_rexmit(struct mip_conn *con, 
                                               u32_t rto)
{
    u16_t count = 0;
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    /* default is MIP_TCP_PERSIST_TIMEOUT */
    if (!con->ctlb.tcp->rtocnt)
    {
        if (rto >= MIP_TCP_RTO_MAX)
        {
            rto = MIP_TCP_RTO_MAX;
        }
        count = rto / MIP_TCP_INTERNAL_TIMEOUT;
        con->ctlb.tcp->rtocnt = count;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_enable_ctltmr
 Description : disable a tcp timer that based on internal 200ms timer.
 Input       : type @ timer's type 
 Output      : None
 Return      : MIP_OK @ means disable ok, other mean faild.
 *****************************************************************************/
static inline int los_mip_tcp_disable_ctltmr(enum tcp_tmr_type type, 
                                             struct mip_conn *con)
{
    int tmptype = type;
    if ((type >= TCP_TMR_MAX) || (type < TCP_TMR_K) 
        || (NULL == con) || (NULL == con->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    con->ctlb.tcp->tmrmask &= (~(0x01<<tmptype));
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_do_retransmit
 Description : process retransmit package when retransmit timer timeout
 Input       : con @ tcp connection's pointer
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_tcp_do_retransmit(struct netif *dev, struct mip_conn *con)
{
    struct mip_tcp_seg *seg = NULL;
    u32_t rto = 0;
//    struct tcp_hdr *tcph = NULL;

    struct ipv4_hdr *iph = NULL;
    u16_t iph_len = 0;
//    int ret = 0;
    
    seg = con->ctlb.tcp->unacked;
//    tcph = con->ctlb.tcp->unacked->tcphdr;
    if (NULL == seg)
    {
        /* no unacked data, so return */
        return ;
    }
    con->ctlb.tcp->rtocnt--;
    if (con->ctlb.tcp->rtocnt)
    {
        /* not timeout */
        return ;
    }

    if (MIP_TRUE == netbuf_get_if_queued(seg->p))
    {
        /* not directly send , because of arp request, so use ipv4_output */
        netbuf_remove_queued(seg->p);
        //repos it and re
        iph = (struct ipv4_hdr *)seg->p->payload;
        iph_len = 4 * MIP_IPHLEN(iph->vhl);
        los_mip_jump_header(seg->p, iph_len);
        los_mip_ipv4_output(dev, seg->p, &dev->ip_addr, 
                              &con->ctlb.tcp->remote_ip, 255, 0, MIP_PRT_TCP);
        /* if the first package not send out ,we give another chance */
        con->ctlb.tcp->rtocnt++;
        seg->rearpcnt++;
        //seg->rexmitcnt++;
    }
    else
    {
        /* direct send the unacked package */
        seg->rexmitcnt++;
        /* calc the next retransmit timeout value */
        rto = los_mip_tcp_backoff_rto(seg->rexmitcnt, con->ctlb.tcp->rto);
        los_mip_tcp_set_tmr_rexmit(con, rto);
        dev->hw_xmit(dev, seg->p);
    }

    if ((seg->rexmitcnt > MIP_TCP_MAX_RETRANS) 
        || (seg->rearpcnt > MIP_TCP_MAX_RETRANS))
    {
        los_mip_tcp_disable_ctltmr(TCP_TMR_REXMIT, con);
        con->ctlb.tcp->rtocnt = 0;
        /* send reset message and tell upper layer tcp closed */
        los_mip_tcp_send_rst(dev, con);
        /* remove first unacked buf */
        los_mip_tcp_remove_seg_unacked(con);
        los_mip_sem_signal(&con->op_finished);
    }
    return ;
}

/*****************************************************************************
 Function    : los_mip_tcp_do_finwait2_timeout
 Description : process fin_wait_2 state timeout
 Input       : con @ tcp connection's pointer
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_tcp_do_finwait2_timeout(struct netif *dev, struct mip_conn *con)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return ;
    }
    con->ctlb.tcp->finwait2--;
    if (!con->ctlb.tcp->finwait2)
    {
        /* fin_wait_2 timed out, we need change to time_wait state */
        los_mip_tcp_disable_ctltmr(TCP_TMR_F, con);
        con->ctlb.tcp->state = TCP_TIME_WAIT;
        /* start 2MSL timer */
        los_mip_tcp_set_tmr_2msl(con, MIP_TCP_2MSL_TIMEOUT);
        los_mip_tcp_enable_ctltmr(TCP_TMR_2MSL, con);
    }
}

/*****************************************************************************
 Function    : los_mip_tcp_do_2msl_timeout
 Description : process time_wait state 2MSL timeout
 Input       : con @ tcp connection's pointer
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_tcp_do_2msl_timeout(struct netif *dev, struct mip_conn *con)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return ;
    }
    con->ctlb.tcp->twomsl--;
    if (!con->ctlb.tcp->twomsl)
    {
        los_mip_tcp_disable_ctltmr(TCP_TMR_2MSL, con);
        con->ctlb.tcp->state = TCP_CLOSED;
        /* send a null data to reveive message box */
        los_mip_snd_msg_to_con(con, NULL);
    }
}

/*****************************************************************************
 Function    : los_mip_tcp_do_persit_timeout
 Description : process persit timer timeout
 Input       : con @ tcp connection's pointer
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_tcp_do_persit_timeout(struct netif *dev, struct mip_conn *con)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        return ;
    }
    con->ctlb.tcp->persist--;
    if (!con->ctlb.tcp->persist)
    {
        los_mip_tcp_send_zeroprobe(dev, con);
        /* reset the persit timeout */
        los_mip_tcp_set_tmr_persit(con, MIP_TCP_PERSIST_TIMEOUT);
    }
}

/*****************************************************************************
 Function    : los_mip_tcp_check_close_now
 Description : check if can send tcp fin_ack segment while in close wait state
 Input       : conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_TRUE means can send now,  MIP_FALSE means don't send now
               need wait timeout to send data.
 *****************************************************************************/
static inline int los_mip_tcp_check_close_now(struct mip_conn *con)
{
    if ((NULL == con) || (NULL == con->ctlb.tcp))
    {
        /* no ethernet device or data to deal with */
        return MIP_FALSE;
    }
    if ((con->ctlb.tcp->state == TCP_CLOSE_WAIT)
        && con->ctlb.tcp->unsndsize == 0)
    {
        return MIP_TRUE;
    }
    return MIP_FALSE;
}

/*****************************************************************************
 Function    : los_mip_tcp_timer_callback
 Description : tcp connection's timer callback.
 Input       : uwPar @ tcp connection's address
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_tcp_timer_callback(u32_t uwPar)
{
    struct mip_conn *con = NULL;
    struct netif *dev = NULL;

    con = (struct mip_conn *)uwPar;
    int i = 0;
    u8_t tmr = 0;
    dev = los_mip_ip_route(&con->ctlb.tcp->remote_ip);
    if ((NULL == con) || (NULL == con->ctlb.tcp)
        || (NULL == dev))
    {
        /* no ethernet device or data to deal with */
        return ;
    }
    
    for (i = 0; i < (int)TCP_TMR_MAX; i++)
    {
        tmr = con->ctlb.tcp->tmrmask & (0x01 << i);
        switch(tmr)
        {
            case TCP_TIMER_KEEPALIVE:
                break;
            case TCP_TIMER_PERSIST:
                los_mip_tcp_do_persit_timeout(dev, con);
                break;
            case TCP_TIMER_FIN_WAIT2:
                los_mip_tcp_do_finwait2_timeout(dev, con);
                break;
            case TCP_TIMER_2MSL:
                los_mip_tcp_do_2msl_timeout(dev, con);
                break;
            case TCP_TIMER_RETRANS:
                los_mip_tcp_do_retransmit(dev, con);
                break;
            default:
                break;
        }
    }
    if (MIP_TRUE == los_mip_tcp_check_send_now(con, 1))
    {
        /* send data immediately */
        los_mip_con_send_tcp_msg(con, TCP_SENDD);
    }
    else
    {
        if (MIP_TRUE == los_mip_tcp_check_close_now(con))
        {
            /* no data need send , and in close_wait state */
            los_mip_tcp_send_fin(dev, con);
            con->ctlb.tcp->state = TCP_LAST_ACK;
        }
    }
    return ;
}
