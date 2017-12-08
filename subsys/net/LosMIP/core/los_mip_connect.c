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
#include "los_mip_udp.h"
#include "los_mip_connect.h"
#include "los_mip_tcpip_core.h"
#include "los_mip_inet.h"

/* mip_skt_mgr is the connection manager */
struct mip_skt_mgr 
{
    int lockflag;/* lock the connection, before delete it, and */
    struct mip_conn *con;/* connnection instance pointer */
};
static struct mip_skt_mgr g_sockets[MIP_CONN_MAX];
/* all port that system have already used stored in g_used_ports */
static u16_t g_used_ports[MIP_CONN_MAX] = {0};

/*****************************************************************************
 Function    : los_mip_init_sockets
 Description : init connection manager, set all intance NULL and unlocked.
 Input       : None
 Output      : None
 Return      : MIP_OK init process done.
 *****************************************************************************/
int los_mip_init_sockets(void)
{
    int i = 0; 
    for (i = 0; i < MIP_CONN_MAX; i++)
    {
        g_sockets[i].lockflag = MIP_SOCKET_UNLOCKED;
        g_sockets[i].con = NULL;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_inc_ref
 Description : increase the connection's reference count
 Input       : con @ connection's pointer   
 Output      : None 
 Return      : MIP_OK process ok, other means failed
 *****************************************************************************/
int los_mip_inc_ref(struct mip_conn *con)
{
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    con->ref++;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dec_ref
 Description : decrease the connection's reference count
 Input       : con @ connection's pointer   
 Output      : None 
 Return      : MIP_OK process ok, other means failed
 *****************************************************************************/
int los_mip_dec_ref(struct mip_conn *con)
{
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    if (con->ref)
    {
        con->ref--;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_walk_con
 Description : return a connection's pointer 
 Input       : finished @ the start index of the connection's array   
 Output      : finished @ the index that not processed  
 Return      : mip_conn * @ connection's pointer
 *****************************************************************************/
struct mip_conn *los_mip_walk_con(s8_t *finished)
{
    int i = 0;
    
    if (NULL == finished)
    {
        return NULL;
    }
    for (i = *finished; i < MIP_CONN_MAX; i++)
    {
        if (NULL != g_sockets[i].con && g_sockets[i].lockflag != MIP_SOCKET_LOCKED)
        {
            *finished = (s8_t)(i+1);
            if (i == MIP_CONN_MAX - 1)
            {
                *finished = -1;
            }
            
            return g_sockets[i].con;
        }
    }
    
    *finished = -1;
    return NULL;
}

/*****************************************************************************
 Function    : los_mip_get_conn
 Description : get connection which will deal with the package.
 Input       : buf @ input package's pointer
               src @ source ip of the package
               dst @ destination of the package
 Output      : None
 Return      :  
 *****************************************************************************/
struct mip_conn* los_mip_tcp_get_conn(struct netbuf *buf, ip_addr_t *src, 
                                      ip_addr_t *dst)
{
    struct mip_conn *ret = NULL;
    struct mip_conn *listen = NULL;
    struct mip_skt_mgr *tmp = NULL;
    u8_t flags = 0;
    int i = 0;
    struct tcp_hdr *tcph = NULL;
    
    if (NULL == buf || NULL == src || NULL == dst)
    {
        /* param err, just return a NULL conn */
        return NULL;
    }
    tcph = (struct tcp_hdr *)buf->payload;
    flags = MIP_TCPH_FLAGS(tcph);
    
    flags = MIP_TCPH_FLAGS(tcph);
    for(i = 0; i < MIP_CONN_MAX; i++)
    {
        tmp = &g_sockets[i];
        if ((NULL == tmp->con) || (NULL == tmp->con->ctlb.tcp))
        {
            continue;
        }
        if ((tmp->con->ctlb.tcp->state == TCP_CLOSED) && !tmp->con->ref)
        {
            /* need free connection */
            los_mip_delete_conn(tmp->con);
            tmp->con = NULL;
            continue;
        }
        if ((tmp->con->type == CONN_TCP)
            && (tmp->con->ctlb.tcp->lport == tcph->dport))
        {
            if ((tmp->con->ctlb.tcp->rport == tcph->sport)
                && (tmp->con->ctlb.tcp->remote_ip.addr == src->addr)
                && (tmp->con->ctlb.tcp->state != TCP_CLOSED))
            {
                if ((tmp->con->ctlb.tcp->localip.addr == INADDR_ANY) 
                    || (tmp->con->ctlb.tcp->localip.addr == dst->addr))
                /* message is for a already created connection */
                ret = tmp->con;
                break;
            }
            else if ((tmp->con->state & STAT_LISTEN)
                     && ((flags & MIP_TCP_SYN) && !(flags & MIP_TCP_ACK)))
            {
                /* message is for creating a new connection, and maybe 
                   it's retransmit package, and the connection already 
                   created.
                */
                listen = tmp->con;
//                break;
            }
        }
    }
    if (NULL == ret)
    {
        /* no connection created, so use listen connection */
        ret = listen;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_port_is_used
 Description : judge if the port input has already be used.
 Input       : port @ the port number that need to process.
 Output      : 
 Return      : MIP_TRUE port is already used. MIP_FALSE not used.
 *****************************************************************************/
int los_mip_port_is_used(u16_t port)
{
    int i = 0;
    for (i  = 0; i < MIP_CONN_MAX; i++)
    {
        if (g_used_ports[i] == port)
        {
            return MIP_TRUE;
        }
    }
    return MIP_FALSE;
}

/*****************************************************************************
 Function    : los_mip_add_port_to_used_list
 Description : add the port number to the used port array.
 Input       : port @ the port number that need to process.
 Output      : 
 Return      : MIP_TRUE add success. -MIP_CON_PORT_RES_USED_OUT no space to 
               store the port number.
 *****************************************************************************/
int los_mip_add_port_to_used_list(u16_t port)
{
    int i = 0;
    if (port == 0)
    {
        return -MIP_ERR_PARAM;
    }
    for (i  = 0; i < MIP_CONN_MAX; i++)
    {
        if(g_used_ports[i] == 0)
        {
            break;
        }
    }
    if (i < MIP_CONN_MAX)
    {
        g_used_ports[i] = port;
    }
    else
    {
        return -MIP_CON_PORT_RES_USED_OUT;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_remove_port_from_used_list
 Description : remove the port number from the used port array
 Input       : port @ the port number that need to process.
 Output      : None
 Return      : MIP_OK process done.
 *****************************************************************************/
int los_mip_remove_port_from_used_list(u16_t port)
{
    int i = 0;
    
    if (port == 0)
    {
        return -MIP_ERR_PARAM;
    }

    for (i  = 0; i < MIP_CONN_MAX; i++)
    {
        if(g_used_ports[i] == port)
        {
            g_used_ports[i] = 0;
            break;
        }
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_generate_port
 Description : generate a port number that unused.
 Input       : None
 Output      : None
 Return      : u16_t @  a port number that generated.
 *****************************************************************************/
u16_t los_mip_generate_port(void)
{
    int i = 0;
    u16_t maxport = 0;
    
    for (i  = 0; i < MIP_CONN_MAX; i++)
    {
        if(maxport < g_used_ports[i] )
        {
            maxport = g_used_ports[i];
        }
    }
    if (maxport == 0)
    {
        maxport = (u16_t)los_mip_random();
    }
    else
    {
        maxport += 1;
    }
    if (maxport < MIP_RANDOM_PORT_BASE_VALUE)
    {
        maxport = MIP_RANDOM_PORT_BASE_VALUE + maxport % 100;
    }
    return MIP_HTONS(maxport);
}

/*****************************************************************************
 Function    : los_mip_conn_init_tcpctl
 Description : init tcp connection control block by default value 
 Input       : ptcp @ tcp connection control block that need initial
 Output      : None
 Return      : MIP_OK @ init ok, other value means failed.
 *****************************************************************************/
static int los_mip_conn_init_tcpctl(struct tcp_ctl *ptcp, struct mip_conn *con)
{
    if (NULL == ptcp)
    {
        return -MIP_ERR_PARAM;
    }
    memset(ptcp, 0, sizeof(struct tcp_ctl));
    
    /* todo: need give the timer callback function pointer */
    ptcp->tmr = los_mip_create_swtimer(MIP_TCP_BASE_TIMER_TIMEOUT,
                                       los_mip_tcp_timer_callback, (u32_t)con);
    if (los_mip_is_valid_timer(ptcp->tmr) != MIP_TRUE)
    {
        return -MIP_TCP_TMR_OPS_ERR;
    }
    los_mip_start_timer(ptcp->tmr);
    ptcp->basecnt = MIP_TCP_INTERNAL_TIMEOUT / MIP_TCP_BASE_TIMER_TIMEOUT;
    ptcp->state = TCP_INITIAL;
    ptcp->isn = los_mip_tcp_gen_isn();
    ptcp->swnd = LOS_MIP_TCP_WIND_SIZE;
    ptcp->unsndsize = 0;
    ptcp->mss = 0;                  /* remote endpoint's mss value */
    ptcp->localmss = MIP_TCP_MSS;   /* local device's mss value */
    ptcp->needackcnt = 0;
    ptcp->rcv_nxt = 0;
    ptcp->rto = MIP_TCP_RTO_MIN;
    ptcp->srtt = 0;
    ptcp->rttvar = 0;
    ptcp->snd_nxt = ptcp->isn;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_conn_deinit_tcpctl
 Description : deinit tcp connection control block by default value 
 Input       : ptcp @ tcp connection control block that need initial
 Output      : None
 Return      : MIP_OK @ deinit ok, other value means failed.
 *****************************************************************************/
static int los_mip_conn_deinit_tcpctl(struct tcp_ctl *ptcp)
{
    struct skt_acpt_msg *msgs = NULL;
    struct mip_tcp_seg *tmp = NULL;
    struct netbuf   *pbuf = NULL;
    if (NULL == ptcp)
    {
        return -MIP_ERR_PARAM;
    }
    
    if (los_mip_is_valid_timer(ptcp->tmr) == MIP_TRUE)
    {
         los_mip_stop_timer(ptcp->tmr);
         los_mip_delete_timer(ptcp->tmr);
    }
    if (los_mip_mbox_valid(&ptcp->acptmbox) == MIP_TRUE)
    {
        while (MIP_OK == los_mip_mbox_tryfetch(&ptcp->acptmbox, (void **)&msgs))
        {
            /* maybe destroy by its self, if socket not -1 not destoryed yet */
            if (msgs->con->socket != -1)
            {
                /* need destroy itsself */
                los_mip_delete_conn(msgs->con);
            }
            msgs->con = NULL;
            /* free message's self */
            los_mpools_free(MPOOL_MSGS, msgs);
            msgs = NULL;
        }
    }
    while(ptcp->unacked != NULL)
    {
        tmp = ptcp->unacked;
        ptcp->unacked = ptcp->unacked->next;
        los_mip_tcp_delte_seg(tmp);
        tmp = NULL;
    }
    while(ptcp->unsend != NULL)
    {
        pbuf = ptcp->unsend;
        ptcp->unsend = ptcp->unsend->next;
        netbuf_free(pbuf);
        pbuf = NULL;
    }
    while(ptcp->recievd != NULL)
    {
        tmp = ptcp->recievd;
        ptcp->recievd = ptcp->recievd->next;
        los_mip_tcp_delte_seg(tmp);
        tmp = NULL;
    }
    memset(ptcp, 0, sizeof(struct tcp_ctl));
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_new_conn
 Description : new a connection and associate a socket with the the connection 
 Input       : type @ the connection type , like udp, tcp ...
 Output      : None
 Return      : mip_conn * @ the connection's pointer
 *****************************************************************************/
struct mip_conn *los_mip_new_conn(enum conn_type type)
{
    int i = 0;
    int pad = 0;
    struct mip_conn *tmp = NULL;
    
    for (i = 0; i < MIP_CONN_MAX; i++)
    {
        if (NULL == g_sockets[i].con)
        {
            break;
        }
    }
    if (i == MIP_CONN_MAX)
    {
        return NULL;
    }
    switch(type)
    {
        case CONN_UDP:
            pad = sizeof(struct udp_ctl);
            break;
        case CONN_TCP:
            pad = sizeof(struct tcp_ctl);
            break;
        default:
            break;
    }
    tmp = (struct mip_conn *)los_mpools_malloc(MPOOL_SOCKET, sizeof(struct mip_conn) + pad);
    if (NULL != tmp)
    {
        memset(tmp, 0, sizeof(struct mip_conn) + pad);
        tmp->socket = i;
        tmp->type = type;
        if(los_mip_mbox_new(&tmp->recvmbox, MIP_CONN_RCVBOX_SIZE) != MIP_OK) 
        {
            //recvmbox box create failed
            while(1);
        }
        tmp->recv_timeout = 0xffffffffU;
        tmp->rcvcnt = 0;
        tmp->backlog = 0;
        los_mip_sem_new(&tmp->op_finished, 1);
        los_mip_sem_wait(&tmp->op_finished, 0);
        switch (type)
        {
            case CONN_UDP:
                tmp->ctlb.udp = (struct udp_ctl *)((u8_t *)tmp + sizeof(struct mip_conn));
                break;
            case CONN_TCP:
                tmp->ctlb.tcp = (struct tcp_ctl *)((u8_t *)tmp + sizeof(struct mip_conn));
                los_mip_conn_init_tcpctl(tmp->ctlb.tcp, tmp);
            default:
                break;
        }
        los_mip_inc_ref(tmp);
    }
    g_sockets[i].con = tmp;
    return g_sockets[i].con;
}

/*****************************************************************************
 Function    : los_mip_clone_conn
 Description : clone a connection from tcp listen connection 
 Input       : tcpcon @ tcp listen connection's pointer
               rip @ new connection's remote ip address
               rport @ new connection's remote port number
 Output      : None
 Return      : mip_conn * @ the new connection's pointer
 *****************************************************************************/
struct mip_conn *los_mip_clone_conn(struct mip_conn *tcpcon, ip_addr_t dst,
                                    ip_addr_t rip, u16_t rport)
{
    int i = 0;
    int pad = 0;
    struct mip_conn *tmp = NULL;
//    int len = 0;
    
    if ((NULL == tcpcon) || (tcpcon->type != CONN_TCP))
    {
        return NULL;
    }
    for (i = 0; i < MIP_CONN_MAX; i++)
    {
        if (NULL == g_sockets[i].con)
        {
            break;
        }
    }
    if (i == MIP_CONN_MAX)
    {
        return NULL;
    }
//    len = sizeof(struct mip_conn);
    
    pad = sizeof(struct tcp_ctl);
    tmp = (struct mip_conn *)los_mpools_malloc(MPOOL_SOCKET, sizeof(struct mip_conn) + pad);
    if (NULL != tmp)
    {
        memset(tmp, 0, sizeof(struct mip_conn) + pad);
        tmp->listen = tcpcon;
        tmp->socket = i;
        tmp->type = tcpcon->type;
        tmp->backlog = 0;
        if(los_mip_mbox_new(&tmp->recvmbox, MIP_CONN_RCVBOX_SIZE) != MIP_OK) 
        {
            //recvmbox box create failed
            while(1);
        }
        tmp->recv_timeout = 0xffffffffU;
        tmp->rcvcnt = 0;
        los_mip_sem_new(&tmp->op_finished, 1);
        los_mip_sem_wait(&tmp->op_finished, 0);
        tmp->ctlb.tcp = (struct tcp_ctl *)((u8_t *)tmp + sizeof(struct mip_conn));
        if (los_mip_conn_init_tcpctl(tmp->ctlb.tcp, tmp) != MIP_OK)
        {
            los_mip_mbox_free(&tmp->recvmbox);
            los_mip_sem_free(&tmp->op_finished);
            los_mpools_free(MPOOL_SOCKET, tmp);
            tmp = NULL;
            return NULL;
        }
        if (tcpcon->ctlb.tcp->localip.addr == INADDR_ANY)
        {
            tmp->ctlb.tcp->localip.addr = dst.addr;
        }
        else
        {
            tmp->ctlb.tcp->localip.addr = tcpcon->ctlb.tcp->localip.addr;
        }
        tmp->ctlb.tcp->lport = tcpcon->ctlb.tcp->lport;
        tmp->ctlb.tcp->remote_ip.addr = rip.addr;
        tmp->ctlb.tcp->rport = rport;
        tmp->ref = 0;
    }
    g_sockets[i].con = tmp;
    return g_sockets[i].con;
}


/*****************************************************************************
 Function    : los_mip_lock_socket
 Description : set the socket locked
 Input       : socket @ socket's handle 
 Output      : None
 Return      : MIP_OK process done
 *****************************************************************************/
int los_mip_lock_socket(int socket)
{
    g_sockets[socket].lockflag = MIP_SOCKET_LOCKED;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_lock_socket
 Description : set the socket unlocked
 Input       : socket @ socket's handle 
 Output      : None
 Return      : MIP_OK process done
 *****************************************************************************/
int los_mip_unlock_socket(int socket)
{
    g_sockets[socket].lockflag = MIP_SOCKET_UNLOCKED;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_set_nonblocking
 Description : set the socket work at nonblocking mode
 Input       : con @ connection's pointer that need set
               val @ the model, > 0 is nonblock model, == 0 is block model
 Output      : None
 Return      : MIP_OK process done
 *****************************************************************************/
int los_mip_set_nonblocking(struct mip_conn *con, int val)
{
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    if (val)
    {
        con->flags |= MIP_CONN_NONBLOCK;
    }
    else
    {
        con->flags &= ~MIP_CONN_NONBLOCK;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_conn_is_nonblocking
 Description : get the socket if it is work at nonblocking mode
 Input       : con @ connection's pointer 
 Output      : None
 Return      : MIP_TRUE connection is work at nonblock model, else block model
 *****************************************************************************/
int los_mip_conn_is_nonblocking(struct mip_conn *con)
{
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    if (con->flags & MIP_CONN_NONBLOCK)
    {
        return MIP_TRUE;
    }
    return MIP_FALSE;
    
}

/*****************************************************************************
 Function    : los_mip_snd_delmsg
 Description : send connection destroy message to tcp/ip core task. 
 Input       : con @ connection's pointer 
 Output      : None
 Return      : MIP_OK send message ok, other send failed  
 *****************************************************************************/
int los_mip_snd_delmsg(struct mip_conn *con)
{
    struct skt_del_msg *msg = NULL;
    struct mip_msg *mipmsg = NULL;

    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    msg = los_mip_new_delmsg(con); 
    if (NULL == msg)
    {
        return -MIP_CON_NEW_MSG_FAILED;
    }
    
    mipmsg = (struct mip_msg *)los_mpools_malloc(MPOOL_MSGS, sizeof(struct mip_msg));
    if (mipmsg == NULL) 
    {
        los_mpools_free(MPOOL_MSGS, msg);
        msg = NULL;
        return -MIP_MPOOL_MALLOC_FAILED;
    }
    memset(mipmsg, 0, sizeof(struct mip_msg));
    mipmsg->type = TCPIP_MSG_DELCON;
    mipmsg->msg.delcon = msg;
    con->cur_msg = mipmsg;
    
    if (los_mip_tcpip_snd_msg(mipmsg) != MIP_OK) 
    {
        /* send delete connection msg failed */
        los_mpools_free(MPOOL_MSGS, msg);
        los_mpools_free(MPOOL_MSGS, mipmsg);
        msg = NULL;
        mipmsg = NULL;
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_delete_conn
 Description : free the memmory that the connection used.and itsself
 Input       : con @ connection's pointer
 Output      : 
 Return      : MIP_OK delete process done.
 *****************************************************************************/
int los_mip_delete_conn(struct mip_conn *con)
{
    int socket = -1;
    struct skt_rcvd_msg *msgs = NULL;
    
    if (NULL == con)
    {
        return MIP_OK;
    }
    socket = con->socket;
    g_sockets[con->socket].con = NULL;
    con->socket = -1;
    while (MIP_OK == los_mip_mbox_tryfetch(&con->recvmbox, (void **)&msgs))
    {
        netbuf_free(msgs->p);
        msgs->p = NULL;
        los_mip_delete_up_sktmsg(msgs);
        msgs = NULL;
    }
    los_mip_mbox_free(&con->recvmbox);
    if (con->type == CONN_TCP)
    {
        los_mip_conn_deinit_tcpctl(con->ctlb.tcp);
        con->ctlb.tcp = NULL;
    }
    //todo if any msgs in acceptmbox
    //los_mip_mbox_free(&con->acceptmbox);
    los_mip_sem_free(&con->op_finished);
    los_mpools_free(MPOOL_SOCKET, con);
    
    los_mip_unlock_socket(socket);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_get_conn
 Description : get the connection's pointer by socket handle
 Input       : socket @ the socket's handle
 Output      : None
 Return      : mip_conn * @ connection's pointer
 *****************************************************************************/
struct mip_conn *los_mip_get_conn(int socket)
{
    if ((socket < 0) || (socket >= MIP_CONN_MAX))
    {
        return NULL;
    }
    if (g_sockets[socket].lockflag == MIP_SOCKET_LOCKED)
    {
        return NULL;
    }
    return g_sockets[socket].con;
}

/*****************************************************************************
 Function    : los_mip_conn_snd
 Description : send out message to tcp/ip core task.
 Input       : data @ the data that need send.
 Output      : None
 Return      : ret @ 0(MIP_OK) send ok, < 0 send failed.
 *****************************************************************************/
int los_mip_conn_snd(struct skt_msg_dat *data)
{
    int ret = 0;
    
    if ((NULL == data) || (NULL == data->con))
    {
        return -MIP_ERR_PARAM;
    }
    switch (data->con->type)
    {
        case CONN_UDP:
            ret = los_mip_udp_output(data->con->ctlb.udp, 
                                     data->p, 
                                     &data->con->ctlb.udp->remote_ip, 
                                     data->con->ctlb.udp->rport); 
            break;
        case CONN_TCP: 
            break;
        default:
            break;
    }
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_new_sktmsg
 Description : create a socket message
 Input       : con @ connection's pointer
               buf @ the data that will stored in the socket message
 Output      : None
 Return      : skt_msg * @ the created socket message's pointer
 *****************************************************************************/
struct skt_msg *los_mip_new_sktmsg(struct mip_conn *con, struct netbuf *buf)
{
    struct skt_msg *msg = NULL;
    
    if ((NULL == con) || (NULL == buf))
    {
        return NULL;
    }
    msg = (struct skt_msg *)los_mpools_malloc(MPOOL_MSGS, sizeof(struct skt_msg));
    if (NULL == msg)
    {
        return NULL;
    }
    memset(msg, 0, sizeof(struct skt_msg));
    msg->sktsndfn = los_mip_conn_snd; 
    msg->data.con = con;
    msg->data.err = 0;
    msg->data.p = buf;
    msg->data.op_finished = &con->op_finished;
    
    return msg;
}

/*****************************************************************************
 Function    : los_mip_new_delmsg
 Description : create a connection delete message
 Input       : con @ connection's pointer
 Output      : None
 Return      : skt_del_msg * @ the created delete message's pointer 
 *****************************************************************************/
struct skt_del_msg *los_mip_new_delmsg(struct mip_conn *con)
{
    struct skt_del_msg *msg = NULL;
    
    if (NULL == con)
    {
        return NULL;
    }
    msg = (struct skt_del_msg *)los_mpools_malloc(MPOOL_MSGS, 
                                                  sizeof(struct skt_del_msg));
    if (NULL == msg)
    {
        return NULL;
    }
    memset(msg, 0, sizeof(struct skt_del_msg));
    msg->con = con; 
    msg->sktdelfn = los_mip_delete_conn;
    
    return msg;
}

/*****************************************************************************
 Function    : los_mip_new_up_sktmsg
 Description : create a connection reciv message
 Input       : buf @ the data that will stored in the recived message
               to @ the dest ip that message send to
               port @ the dest port number 
 Output      : None
 Return      : skt_rcvd_msg * @ the created socket recive message's pointer
 *****************************************************************************/
struct skt_rcvd_msg *los_mip_new_up_sktmsg(struct netbuf *buf, ip_addr_t *to, u16_t port)
{
    struct skt_rcvd_msg *msg = NULL;
    
    if ((NULL == buf) || (NULL == to))
    {
        return NULL;
    }
    msg = (struct skt_rcvd_msg *)los_mpools_malloc(MPOOL_MSGS, sizeof(struct skt_rcvd_msg));
    if (NULL == msg)
    {
        return NULL;
    }
    memset(msg, 0, sizeof(struct skt_rcvd_msg));
    msg->p = buf;
    msg->addr.addr = to->addr;
    msg->port = port;
    
    return msg;
}

/*****************************************************************************
 Function    : los_mip_delete_up_sktmsg
 Description : free the socket recived message's memory 
 Input       : msg @ the message's point that will freed.
 Output      : None
 Return      : 0(MIP_OK) is free ok. other is failed.
 *****************************************************************************/
int los_mip_delete_up_sktmsg(struct skt_rcvd_msg *msg)
{
    int ret = MIP_OK;
    if (NULL == msg)
    {
        return -MIP_ERR_PARAM;
    }
    ret = los_mpools_free(MPOOL_MSGS, msg);
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_snd_sktmsg
 Description : send socket message to tcp/ip core task
 Input       : con @ connection's pointer
               buf @ the data's pointer that will stored in the socket message
 Output      : None
 Return      : MIP_OK send ok, other value means send failed.
 *****************************************************************************/
int los_mip_snd_sktmsg(struct mip_conn *con, struct netbuf *buf)
{
    struct skt_msg *msg = NULL;
    struct mip_msg *mipmsg = NULL;
    u32_t ret = 0;
    
    if ((NULL == con) || (NULL == buf))
    {
        return -MIP_ERR_PARAM;
    }
    msg = los_mip_new_sktmsg(con, buf); 
    if (NULL == msg)
    {
        return -MIP_CON_NEW_MSG_FAILED;
    }
    
    mipmsg = (struct mip_msg *)los_mpools_malloc(MPOOL_MSGS, sizeof(struct mip_msg));
    if (mipmsg == NULL) 
    {
        los_mpools_free(MPOOL_MSGS, msg);
        msg = NULL;
        return -MIP_MPOOL_MALLOC_FAILED;
    }
    memset(mipmsg, 0, sizeof(struct mip_msg));
    mipmsg->type = TCPIP_MSG_SKT;
    mipmsg->msg.conmsg = msg;
    con->cur_msg = mipmsg;
    
    if (los_mip_tcpip_snd_msg(mipmsg) != MIP_OK) 
    {
        los_mpools_free(MPOOL_MSGS, msg);
        los_mpools_free(MPOOL_MSGS, mipmsg);
        msg = NULL;
        mipmsg = NULL;
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    /* timeout just here for test ,need modified */
    ret = los_mip_sem_wait(msg->data.op_finished, 1000);
    if (ret == MIP_OS_TIMEOUT)
    {
        los_mpools_free(MPOOL_MSGS, msg);
        los_mpools_free(MPOOL_MSGS, mipmsg);
        msg = NULL;
        mipmsg = NULL;
        return -MIP_SOCKET_SND_TIMEOUT;
    }

    netbuf_free(buf);
    los_mpools_free(MPOOL_MSGS, msg);
    los_mpools_free(MPOOL_MSGS, mipmsg);
    msg = NULL;
    mipmsg = NULL;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_new_acptmsg
 Description : create a accept message 
 Input       : con @ connection's pointer
 Output      : None
 Return      : msg @ accept message's pointer, if failed  return NULL
 *****************************************************************************/
struct skt_acpt_msg *los_mip_new_acptmsg(struct mip_conn *con)
{
    struct skt_acpt_msg *msg = NULL;
    if (NULL == con)
    {
        return NULL;
    }
    msg = (struct skt_acpt_msg *)los_mpools_malloc(MPOOL_MSGS, 
                                                   sizeof(struct skt_acpt_msg));
    if (NULL == msg)
    {
        return NULL;
    }
    memset(msg, 0, sizeof(struct skt_acpt_msg));
    msg->con = con;
    
    return msg;
}

/*****************************************************************************
 Function    : los_mip_snd_acptmsg_to_listener
 Description : send accept message to listen socket.
 Input       : listener @ listern connection's pointer
               msg @ the recived message's pointer
 Output      : None
 Return      : MIP_OK send ok, other value means failed.
 *****************************************************************************/
int los_mip_snd_acptmsg_to_listener(struct mip_conn *listener, 
                                    struct skt_acpt_msg *msg)
{
    if ((NULL == listener) || (NULL == msg))
    {
        return -MIP_ERR_PARAM;
    }
    if (!(listener->state & STAT_LISTEN))
    {
        /* not listen connection */
        return -MIP_ERR_PARAM;
    }
    if (los_mip_mbox_trypost(&listener->ctlb.tcp->acptmbox, msg) != MIP_OK) 
    {
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_snd_msg_to_con
 Description : send recived message to connection.
 Input       : con @ connection's pointer
               msg @ the recived message's pointer
 Output      : None
 Return      : MIP_OK send ok, other value means failed.
 *****************************************************************************/
int los_mip_snd_msg_to_con(struct mip_conn *con, void *msg)
{
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    /* if NULL == msg , it means tcp closed */
    if (los_mip_mbox_trypost(&con->recvmbox, msg) != MIP_OK) 
    {
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    con->rcvcnt++;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_add_rcv_msg_to_tail
 Description : add recived message to the tail of connection recived messages.
 Input       : con @ connection's pointer
               msg @ recived message's pointer
 Output      : None
 Return      : MIP_OK process done ok. other value means failed.
 *****************************************************************************/
int los_mip_add_rcv_msg_to_tail(struct mip_conn *con, struct skt_rcvd_msg *msg)
{
    struct skt_rcvd_msg *tmp = NULL;
    
    if ((NULL == con) || (NULL == msg))
    {
        return -MIP_ERR_PARAM;
    }
    if (NULL == con->rcvdata)
    {
        con->rcvdata = msg;
    }
    else
    {
        tmp = con->rcvdata;
        while (NULL != tmp->next)
        {
            tmp = tmp->next;
        }
        tmp->next = msg;
    }
    msg->next = NULL;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_con_set_rcv_timeout
 Description : set recive message wait timeout value
 Input       : con @ connection's pointer
               tm @ time struct pointer
               len @ the time struct's size
 Output      : None
 Return      : MIP_OK set ok, other valude means set failed.
 *****************************************************************************/
int los_mip_con_set_rcv_timeout(struct mip_conn *con, const void *tm, u32_t len) 
{
    struct timeval *val = NULL;
    u32_t delaytm = 0;
    
    if ((NULL == con) || (NULL == tm) || (len == 0))
    {
        return -MIP_ERR_PARAM;
    }
    if (len != sizeof(struct timeval)) 
    {
        return -MIP_ERR_PARAM;
    }
    val = (struct timeval *)tm;
    
    delaytm = val->tv_sec* 1000 + val->tv_usec / 1000;
    
    con->recv_timeout = delaytm;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_udp_bind
 Description : do bind function. set the ip and port which the conneciotn
               will bind.
 Input       : udpctl @ udp connection private data's pointer
               dst_ip @ ip address the udp connection will bind
               dst_port @ port number the udp connection will bind
 Output      : None
 Return      : MIP_OK bind process ok. other value means error happened.
 *****************************************************************************/
int los_mip_udp_bind(struct udp_ctl *udpctl, u32_t *dst_ip, u16_t dst_port)
{
    if ((NULL == udpctl) || (NULL == dst_ip))
    {
         return -MIP_ERR_PARAM;
    }
    if (MIP_TRUE == los_mip_port_is_used(dst_port))
    {
        return -MIP_CON_PORT_ALREADY_USED;
    }
    if (MIP_OK != los_mip_add_port_to_used_list(dst_port))
    {
        return -MIP_CON_PORT_RES_USED_OUT;
    }
    /* note there are some bugs, for ipv6 */
    udpctl->localip.addr = *dst_ip;
    udpctl->lport = dst_port;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_conn_bind
 Description : do bind function. set the ip and port which the conneciotn
               will bind.
 Input       : mip_conn @ connection's pointer
               dst_ip @ ip address the udp connection will bind
               dst_port @ port number the udp connection will bind
 Output      : None
 Return      : MIP_OK bind process ok. other value means error happened.
 *****************************************************************************/
int los_mip_conn_bind(struct mip_conn *conn, u32_t *dst_ip, u16_t dst_port)
{
    if ((NULL == conn) || (NULL == dst_ip))
    {
         return -MIP_ERR_PARAM;
    }
    if (MIP_TRUE == los_mip_port_is_used(dst_port))
    {
        return -MIP_CON_PORT_ALREADY_USED;
    }
    if (MIP_OK != los_mip_add_port_to_used_list(dst_port))
    {
        return -MIP_CON_PORT_RES_USED_OUT;
    }
    if (conn->type == CONN_TCP)
    {
        /* note there are some bugs, for ipv6 */
        conn->ctlb.tcp->localip.addr = *dst_ip;
        conn->ctlb.tcp->lport = dst_port;
    }
    if (conn->type == CONN_UDP)
    {
        /* note there are some bugs, for ipv6 */
        conn->ctlb.udp->localip.addr = *dst_ip;
        conn->ctlb.udp->lport = dst_port;
    }
    return MIP_OK;
}
/*****************************************************************************
 Function    : los_mip_con_new_tcp_msg
 Description : create a tcp message.
 Input       : mip_conn @ connection's pointer
               type @ tcp message type for operation
 Output      : None
 Return      : msg @ the tcp message's pointer. if failed return NULL
 *****************************************************************************/
static struct skt_tcp_msg* los_mip_con_new_tcp_msg(struct mip_conn *conn, 
                                                   enum tcp_msg_type type)
{
    struct skt_tcp_msg* msg = NULL;
    msg = (struct skt_tcp_msg*)los_mpools_malloc(MPOOL_MSGS, 
                                                 sizeof(struct skt_tcp_msg));
    if (NULL != msg)
    {
        msg->con = conn;
        msg->type = type;
    }
    return msg;
}

/*****************************************************************************
 Function    : los_mip_con_send_tcp_msg
 Description : send tcp message to tcp/ip core task. the core task will call 
               tcp function to do connect, close, send ....
 Input       : mip_conn @ connection's pointer
               type @ tcp message type that indicate the function that need
               to call.
 Output      : None
 Return      : MIP_OK send message ok. other value means error happened.
 *****************************************************************************/
int los_mip_con_send_tcp_msg(struct mip_conn *conn, enum tcp_msg_type type)
{
    struct skt_tcp_msg* msg = NULL;
    struct mip_msg *mipmsg = NULL;

    if (NULL == conn)
    {
        return -MIP_ERR_PARAM;
    }
    msg = los_mip_con_new_tcp_msg(conn, type); 
    if (NULL == msg)
    {
        return -MIP_CON_NEW_MSG_FAILED;
    }
    
    mipmsg = (struct mip_msg *)los_mpools_malloc(MPOOL_MSGS, sizeof(struct mip_msg));
    if (mipmsg == NULL) 
    {
        los_mpools_free(MPOOL_MSGS, msg);
        msg = NULL;
        return -MIP_MPOOL_MALLOC_FAILED;
    }
    memset(mipmsg, 0, sizeof(struct mip_msg));
    mipmsg->type = TCPIP_MSG_TCP;
    mipmsg->msg.tcpmsg = msg;
    conn->cur_msg = mipmsg;
    
    if (los_mip_tcpip_snd_msg(mipmsg) != MIP_OK) 
    {
        /* send delete connection msg failed */
        los_mpools_free(MPOOL_MSGS, msg);
        los_mpools_free(MPOOL_MSGS, mipmsg);
        msg = NULL;
        mipmsg = NULL;
        return -MIP_OSADP_MSGPOST_FAILED;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_do_connect
 Description : do connect function. set remote ip and port which the conneciotn
               will connect to.
 Input       : mip_conn @ connection's pointer
               dst_ip @ ip address the udp connection will bind
               dst_port @ port number the udp connection will bind
 Output      : None
 Return      : MIP_OK connect process ok. other value means error happened.
 *****************************************************************************/
int los_mip_do_connect(struct mip_conn *conn, u32_t *dst_ip, u16_t dst_port)
{
    int sret = 0;
    u32_t ret = 0;
    if ((NULL == conn) || (NULL == dst_ip))
    {
         return -MIP_ERR_PARAM;
    }
    conn->state = STAT_CONNECT;
    if (conn->type == CONN_TCP)
    {
        /* note there are some bugs, for ipv6 */
        if (conn->ctlb.tcp->remote_ip.addr == INADDR_ANY)
        {
            conn->ctlb.tcp->remote_ip.addr = *dst_ip;
            conn->ctlb.tcp->rport = dst_port;
        }
        if (conn->ctlb.tcp->lport == 0x00)
        {
            /* socket not bind, so generate the local random port */
            conn->ctlb.tcp->lport = los_mip_generate_port();
            if (MIP_OK != los_mip_add_port_to_used_list(conn->ctlb.tcp->lport))
            {
                return -MIP_CON_PORT_RES_USED_OUT;
            }
        }
        /* send syn message out to do tcp connect process */
        sret = los_mip_con_send_tcp_msg(conn, TCP_CONN);
        if (sret != MIP_OK)
        {
            /* send connect message to core failed */
            return -1;
        }
        ret = los_mip_sem_wait(&conn->op_finished, conn->recv_timeout);
        if (ret == MIP_OS_TIMEOUT)
        {
            /* connect failed */
            return -1;
        }
        if (conn->ctlb.tcp->state != TCP_ESTABLISHED)
        {
            /* connect failed */
            return -1;
        }
    }
    if (conn->type == CONN_UDP)
    {
        /* note there are some bugs, for ipv6 */
        conn->ctlb.udp->remote_ip.addr = *dst_ip;
        conn->ctlb.udp->rport = dst_port;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_listen
 Description : create tcp connect listen resources.
 Input       : mip_conn @ tcp connection's pointer
               backlog @ limit the number of outstanding connections 
               in the socket's listen queue
 Output      : None
 Return      : MIP_OK listen process ok. other value means error happened.
 *****************************************************************************/
int los_mip_tcp_listen(struct mip_conn *conn, int backlog)
{
    if ((NULL == conn) || (backlog < 0))
    {
        return -MIP_ERR_PARAM;
    }
    conn->state = STAT_LISTEN;
    if(los_mip_mbox_new(&conn->ctlb.tcp->acptmbox, backlog) != MIP_OK)
    {
        /* acceptmbox create failed */
        return -MIP_TCP_LISTEN_MBOX_ERR;
    }
    /* the backlog is < 255, if > 255 the backlog should defined as u32_t */
    conn->backlog = ((backlog & 0x00ff) << 8);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_shut_slide_wnd
 Description : shutdown the sliding window
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK @ process ok. other value means error happened.
 *****************************************************************************/
static inline int los_mip_tcp_shut_slide_wnd(struct mip_conn *conn)
{
    if (NULL == conn || NULL == conn->ctlb.tcp)
    {
        return -MIP_ERR_PARAM;
    }
    /* todo: need do resources protect */
    conn->ctlb.tcp->swnd = LOS_MIP_TCP_WIND_SIZE;
    return MIP_OK;
}
/*****************************************************************************
 Function    : los_mip_tcp_wait_closed
 Description : wait the connetion's state changed to closed.
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK @ process ok. other value means error happened.
 *****************************************************************************/
static inline int los_mip_tcp_wait_closed(struct mip_conn *conn, u32_t timeout)
{
    int retry = timeout / 5;
    if (NULL == conn || NULL == conn->ctlb.tcp)
    {
        return -MIP_ERR_PARAM;
    }
    if (!retry)
    {
        retry = 1;
    }
    while(conn->ctlb.tcp->state != TCP_CLOSED)
    {
        los_mip_delay(5);
        retry--;
        if (retry == 0)
        {
            break;
        }
    }
    return MIP_OK;
}
/*****************************************************************************
 Function    : los_mip_tcp_close
 Description : close a tcp connection.
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK process ok. other value means error happened.
 *****************************************************************************/
int los_mip_tcp_close(struct mip_conn *conn)
{
    u32_t ret = 0;
    if (NULL == conn || NULL == conn->ctlb.tcp)
    {
        return -MIP_ERR_PARAM;
    }
    /*
        1: make receive sliding windows empty 
        2: discard all send buf's data.
        3: send fin message. 
        4: wait connetion state to closed.
        5: release connection's memory
    */
    los_mip_tcp_shut_slide_wnd(conn);
    if ((conn->ctlb.tcp->state <= TCP_ESTABLISHED)
        && (conn->ctlb.tcp->state > TCP_CLOSED))
    {
        los_mip_con_send_tcp_msg(conn, TCP_CLOSE);
    }
    /* wait 100ms for tcp changed to closed */
    los_mip_tcp_wait_closed(conn, 100);
    los_mip_lock_socket(conn->socket);
    ret = los_mip_snd_delmsg(conn);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_tcp_read
 Description : read data from tcp receive buffer.
 Input       : mip_conn @ tcp connection's pointer
               mem @ the buffer that store the read data
               len @ the length that user want to read 
 Output      : None
 Return      : copylen @ the data length that read out from the tcp buffer.
               if connection is closed , return -1, if no data return 0
 *****************************************************************************/
int los_mip_tcp_read(struct mip_conn *conn, void *mem, size_t len)
{
    mip_mbox_t *recvmbox = NULL;
    int recv_timeout = 0;
    void *tcpmsgs = NULL;
    struct mip_tcp_seg *head = NULL;
    struct mip_tcp_seg *next = NULL;
    int ret = 0;
    int copylen = 0;
    
    if ((NULL == conn) || (NULL == mem))
    {
        return -MIP_ERR_PARAM;
    }
    if(conn->ctlb.tcp->state == TCP_CLOSED)
    {
        /* todo: should set errno that connection closed */
        return -1;
    }
    if(conn->ctlb.tcp->flag & TCP_RXCLOSED)
    {
        /* todo: should set errno that connection rx shutdown */
        return -1;
    }
    recvmbox = &conn->recvmbox;
    recv_timeout = conn->recv_timeout;
    conn->state |= STAT_RD;
    los_mip_con_send_tcp_msg(conn, TCP_READ);
    if (MIP_TRUE == los_mip_conn_is_nonblocking(conn))
    {
        ret = los_mip_mbox_fetch(recvmbox, (void **)&tcpmsgs, 1);
    }
    else
    {
        ret = los_mip_mbox_fetch(recvmbox, (void **)&tcpmsgs, recv_timeout);
    }
    conn->state &= (~STAT_RD);
    if (MIP_OK != ret)
    {
        /* fetch message box error */
        return copylen;
    }
    head = (struct mip_tcp_seg *)tcpmsgs;
    if (NULL == head)
    {
       /* this means connection closed interal, we need return -1*/
       copylen = -1;
    }
    else
    {
        if (len < head->len)
        {
            copylen = len;
            memcpy(mem, head->p->payload, copylen);
            head->p->payload = (u8_t *)head->p->payload + len;
            head->p->len -= len;
            head->len -= len;
        }
        else
        {
            copylen = 0;
            while ((NULL != head) && copylen < len)
            {
                if (copylen + head->len > len)
                {
                    memcpy((u8_t *)mem + copylen, head->p->payload, len - copylen);
                    head->len -= (len - copylen); 
                    head->p->len -= (len - copylen); 
                    copylen = len;
                }
                else
                {
                    next = head->next;
                    memcpy((u8_t *)mem + copylen, head->p->payload, head->len);
                    copylen += head->len;
                    /* free the read out segment's buffer */
                    los_mip_tcp_delte_seg(head);
                    head = next;
                    conn->ctlb.tcp->recievd = next;
                }
            }
            if (NULL == conn->ctlb.tcp->recievd)
            {
                conn->rcvcnt = 0;
            }
        }
        /* todo: need do some protect */
        conn->ctlb.tcp->swnd += copylen;
        /* todo: maybe need tell remote the slide window changed */
    }
    return copylen;
}

/*****************************************************************************
 Function    : los_mip_tcp_write_partly
 Description : write a part of user's data to tcp send buffer.
 Input       : mip_conn @ tcp connection's pointer
               mem @ data's address
               len @ the data length that stored in the mem buffer. 
 Output      : None
 Return      : cpylen @ the copyed data length.
 *****************************************************************************/
static int los_mip_tcp_write_partly(struct mip_conn *conn, const void *mem, 
                                    size_t len)
{
    int cpylen = 0;
    struct netbuf *tmp = NULL;
    struct netbuf *tail = NULL;
    /*get the min segment size */
    tmp = netbuf_alloc(NETBUF_RAW, len);
    if (NULL != tmp)
    {
        cpylen = len;
        memcpy(tmp->payload, mem, len);
        tmp->len = len;
        tail = conn->ctlb.tcp->unsend;
        if (NULL == tail)
        {
            conn->ctlb.tcp->unsend = tmp;
        }
        else
        {
            while(NULL != tail->next)
            {
                tail = tail->next;
            }
            tail->next = tmp;
        }
        tmp->next = NULL;
    }
    return cpylen;
}

/*****************************************************************************
 Function    : los_mip_tcp_write
 Description : write data to tcp send buffer.
 Input       : mip_conn @ tcp connection's pointer
               mem @ data's address
               len @ the data length that stored in the mem buffer. 
 Output      : None
 Return      : wlen @ the real length that write to the send buffer.
 *****************************************************************************/
int los_mip_tcp_write(struct mip_conn *conn, const void *mem, size_t len)
{
    int wlen = 0;
    int ret = 0;
    int tmpwr = 0;
    int tmpunsnd = 0;
    char *tmpmem = NULL;
    if ((NULL == conn) || (NULL == mem)
        || (NULL == conn->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    if((conn->ctlb.tcp->flag & TCP_TXCLOSED)
        || (conn->ctlb.tcp->state == TCP_CLOSED))
    {
        /* todo: should set errno that connection tx shutdown */
        return -1;
    }
    /* copy the data to tcp send buffer */
    if (MIP_TRUE != los_mip_conn_is_nonblocking(conn))
    {
        /* block mode */
        while(tmpwr < len)
        {
            /* todo: need do some protect for the unsndsize */
            tmpunsnd = conn->ctlb.tcp->unsndsize;
            if (tmpunsnd < LOS_MIP_TCP_SND_BUF_SIZE)
            {
                /* copy part data to send buffer */
                wlen = LOS_MIP_TCP_SND_BUF_SIZE - tmpunsnd;
                tmpmem = (char *)mem + tmpwr;
                if (len > wlen)
                {
                    ret = los_mip_tcp_write_partly(conn, tmpmem, wlen);
                }
                else
                {
                    wlen = len;
                    ret = los_mip_tcp_write_partly(conn, tmpmem, wlen);
                }
                if (ret != wlen)
                {
                    /* error happend need wait some timer */
                    wlen = ret;
                    los_mip_delay(10);
                }
                /* update the un-send data length */
                conn->ctlb.tcp->unsndsize += wlen;
                tmpwr += wlen;
            }
            else
            {
                /* no space for data send , sleep some time and */
                los_mip_delay(10);
            }
        }
        wlen = len;
    }
    else
    {
        /* non-block mode */
        /* todo: need do some protect for the unsndsize */
        tmpunsnd = conn->ctlb.tcp->unsndsize;
        if (tmpunsnd < LOS_MIP_TCP_SND_BUF_SIZE)
        {
            /* copy part data to send buffer */
            wlen = LOS_MIP_TCP_SND_BUF_SIZE - tmpunsnd;
            if (len > wlen)
            {
                ret = los_mip_tcp_write_partly(conn, mem, wlen);
            }
            else
            {
                wlen = len;
                ret = los_mip_tcp_write_partly(conn, mem, wlen);
            }
            //ret = los_mip_tcp_write_partly(conn, mem, wlen);
            if (ret != wlen)
            {
                wlen = ret;
            }
            /* update the un-send data length */
            conn->ctlb.tcp->unsndsize += wlen;
        }
        else
        {
            /* no space for data send , should return 0*/
        }
    }
    if (wlen)
    {
        /* tell tcp stack to send data */
        los_mip_con_send_tcp_msg(conn, TCP_SENDI);
    }
    return wlen;
}

/*****************************************************************************
 Function    : los_mip_tcp_quickack_enable
 Description : enable tcp quick ack mode .
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK @ process ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_quickack_enable(struct mip_conn *conn)
{
    if ((NULL == conn) || (NULL == conn->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    conn->ctlb.tcp->flag |= TCP_QUICKACK;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_quickack_disable
 Description : disable tcp quick ack mode.
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK @ process ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_quickack_disable(struct mip_conn *conn)
{
    if ((NULL == conn) || (NULL == conn->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    conn->ctlb.tcp->flag &= ~TCP_QUICKACK;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_nagle_disable
 Description : disable nagle algorithm.
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK @ process ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_nagle_disable(struct mip_conn *conn)
{
    if ((NULL == conn) || (NULL == conn->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    conn->ctlb.tcp->flag |= TCP_NODELAY;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_nagle_enable
 Description : enable nagle algorithm.
 Input       : mip_conn @ tcp connection's pointer
 Output      : None
 Return      : MIP_OK @ process ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_nagle_enable(struct mip_conn *conn)
{
    if ((NULL == conn) || (NULL == conn->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    conn->ctlb.tcp->flag &= ~TCP_NODELAY;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_do_shutdown
 Description : shutdown the connection's read/write .
 Input       : mip_conn @ tcp connection's pointer
               how @ the type of shutdown
 Output      : None
 Return      : MIP_OK @ process ok, other value means failed.
 *****************************************************************************/
int los_mip_tcp_do_shutdown(struct mip_conn *conn, int how)
{
    int shutrx = 0;
    int shutwr = 0;
    
    if ((NULL == conn) || (NULL == conn->ctlb.tcp))
    {
        return -MIP_ERR_PARAM;
    }
    
    shutrx = how & MIP_SHUT_RD;
    shutwr = how & MIP_SHUT_WR;
    if (shutrx)
    {
        conn->ctlb.tcp->flag |= TCP_RXCLOSED;
    }
    if (shutwr)
    {
        conn->ctlb.tcp->flag |= TCP_TXCLOSED;
        los_mip_con_send_tcp_msg(conn, TCP_CLOSE);
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_tcp_get_unaccept_conn
 Description : get un-accepted .
 Input       : conn @ tcp connection's pointer
               type @ the tcp message type
 Output      : None
 Return      : ret @ 0 means send ok, other value means error happened.
 *****************************************************************************/
struct mip_conn * los_mip_tcp_get_unaccept_conn(struct mip_conn *listen)
{
    int i = 0; 
    struct mip_conn *tmp = NULL;
    for (i = 0; i < MIP_CONN_MAX; i++)
    {
        tmp = g_sockets[i].con;
        if ((NULL != tmp)
            && (tmp->listen == listen)
            && (tmp->ref == 0)
            && (tmp->ctlb.tcp->state > TCP_CLOSED))
        {
            return tmp;
        }
    }
    return NULL;
}
