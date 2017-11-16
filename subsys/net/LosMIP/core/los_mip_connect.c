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
        default:
            break;
    }
    tmp = (struct mip_conn *)los_mpools_malloc(MPOOL_SOCKET, sizeof(struct mip_conn) + pad);
    //g_sockets[i] = (struct mip_conn *)los_mpools_malloc(MPOOL_SOCKET, sizeof(struct mip_conn) + pad);
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
        //to do it while listening the connection
//        if(los_mip_mbox_new(&g_sockets[i]->acceptmbox, MIP_CONN_ACPTBOX_SIZE) != MIP_OK) 
//        {
//            //acceptmbox create failed
//            while(1);
//        }
        tmp->recv_timeout = 0xffffffffU;
        tmp->rcvcnt = 0;
        los_mip_sem_new(&tmp->op_finished, 1);
        los_mip_sem_wait(&tmp->op_finished, 0);
        switch (type)
        {
            case CONN_UDP:
                tmp->ctlb.udp = (struct udp_ctl *)((u8_t *)tmp + sizeof(struct mip_conn));
                break;
            default:
                break;
        }
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
    while (MIP_OK == los_mip_mbox_tryfetch(&con->recvmbox, (void **)&msgs))
    {
        netbuf_free(msgs->p);
        msgs->p = NULL;
        los_mip_delete_up_sktmsg(msgs);
        msgs = NULL;
    }
    los_mip_mbox_free(&con->recvmbox);
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
            ret = los_mip_udp_output(data->con->ctlb.udp, data->p, &data->con->ctlb.udp->remote_ip, data->con->ctlb.udp->rport); 
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
 Function    : los_mip_snd_msg_to_con
 Description : send recived message to connection.
 Input       : con @ connection's pointer
               msg @ the recived message's pointer
 Output      : None
 Return      : MIP_OK send ok, other value means failed.
 *****************************************************************************/
int los_mip_snd_msg_to_con(struct mip_conn *con, struct skt_rcvd_msg *msg)
{
    if ((NULL == con) || (NULL == msg))
    {
        return -MIP_ERR_PARAM;
    }

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
