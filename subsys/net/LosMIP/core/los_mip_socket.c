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
#include "los_mip_socket.h"

/*****************************************************************************
 Function    : los_mip_socket
 Description : compatible socket interface, create a socket 
 Input       : domain @ Specifies the communications domain in which a 
                        socket is to be created
               type @ Specifies the type of socket to be created
               protocol @ Specifies a particular protocol to be used 
                          with the socket
 Output      : None
 Return      : socket @ socket's handle.
 *****************************************************************************/
int los_mip_socket(int domain, int type, int protocol)
{
    int socket = -1;
    struct mip_conn *con = NULL;
    
    switch (domain)
    {
        case AF_INET:
            break;
        default:
            return -MIP_SOCKET_NOT_SUPPORTED;
    }
    switch (type)
    {
        case SOCK_DGRAM:
            if ((IPPROTO_UDP == protocol) || (0 == protocol))
            {
                con = los_mip_new_conn(CONN_UDP);
            }
            break;
        case SOCK_STREAM:
            if ((IPPROTO_TCP == protocol) || (0 == protocol))
            {
                con = los_mip_new_conn(CONN_TCP);
            }
            break;
        default:
            return -MIP_SOCKET_NOT_SUPPORTED;
    }
    if (NULL != con)
    {
        socket = con->socket;
    }
    
    return socket;
}

/*****************************************************************************
 Function    : los_mip_close
 Description : compatible socket interface. close a socket
 Input       : s @ socket's handle.
 Output      : None
 Return      : 0(MIP_OK) close ok, <0 close failed.
 *****************************************************************************/
int los_mip_close(int s)
{
    int ret = MIP_OK;
    struct mip_conn *con = NULL;
    int type = 0;
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return MIP_OK;
    }
    type = con->type;
    
    los_mip_lock_socket(s);
    switch(type)
    {
        case CONN_UDP:
            ret = los_mip_snd_delmsg(con);
            break;
        case CONN_TCP:
            ret = los_mip_tcp_close(con);
            break;
        default:
            break;
    }

    return ret;
}

/*****************************************************************************
 Function    : los_mip_read
 Description : compatible socket interface. read data from a socket
 Input       : s @ socket's handle 
               mem @ the buf which to store the read data
               len @ buf length
 Output      : None
 Return      : ret @ data's length that read from socket.
 *****************************************************************************/
int los_mip_read(int s, void *mem, size_t len)
{
    int ret = 0;
    if ((NULL == mem) || (len <= 0) || (s < 0))
    {
        return -MIP_ERR_PARAM;
    }
    ret = los_mip_recvfrom(s, mem, len, 0, NULL, NULL);
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_read
 Description : compatible socket interface. write data to a socket
 Input       : s @ socket's handle 
               data @ the buf which store the write data
               size @ data length
 Output      : None
 Return      : len @ data's length that send out.
 *****************************************************************************/
int los_mip_write(int s, const void *data, size_t size)
{
    int len = 0;
	struct mip_conn *con = NULL;
    
    if ((NULL == data) || (size <= 0) || (s < 0))
    {
        return -MIP_ERR_PARAM;
    }
    
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return MIP_OK;
    }
    if (CONN_UDP == con->type)
    {
        /* udp can't use write function */
        return -1;
    }
    if (CONN_TCP == con->type)
    {
        if ((con->ctlb.tcp->state == TCP_ESTABLISHED) 
            || ((con->ctlb.tcp->state == TCP_CLOSE_WAIT)))
        {
            /* call tcp write function */
            len = los_mip_tcp_write(con, data, size);
        }
        else
        {
            /* todo: tcp not established, should set errno */
            return -1;
        }

    }
    /* todo: add tcp send code here */
    return len;
}

/*****************************************************************************
 Function    : los_mip_recv
 Description : compatible socket interface. recive data from a socket
 Input       : s @ socket's handle 
               mem @ the buf which store the recived data
               len @ buf size
               flags @ Specifies the type of message reception
 Output      : None
 Return      : ret @ data's length that recived.
 *****************************************************************************/
int los_mip_recv(int s, void *mem, size_t len, int flags)
{
    int ret = 0;
    if ((s < 0) || (NULL == mem))
    {
        return -MIP_ERR_PARAM;
    }
    ret = los_mip_recvfrom(s, mem, len, flags, NULL, NULL);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_recvfrom
 Description : compatible socket interface. recive data from a socket
 Input       : s @ socket's handle 
               mem @ the buf which store the recived data
               len @ buf size
               flags @ Specifies the type of message reception
               from @ source ip address
               fromlen @ length of the *from
 Output      : None
 Return      : copylen @ data's length that recived.
 *****************************************************************************/
int los_mip_recvfrom(int s, void *mem, 
                    size_t len, int flags,
                    struct sockaddr *from, 
                    socklen_t *fromlen) 
{
    struct mip_conn *con = NULL;
    struct skt_rcvd_msg *msgs = NULL;
//    void *tcpmsgs = NULL;
//    struct mip_tcp_seg *head = NULL;
    int ret = MIP_OK;
    struct sockaddr_in in;
    size_t copylen = 0;
    mip_mbox_t *recvmbox = NULL;
    int recv_timeout = 0;
    enum conn_type type = CONN_MAX;
    
    if ((s < 0) || (NULL == mem))
    {
        return -MIP_ERR_PARAM;
    }
    
    (void)flags;

    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -1;
    }
    recvmbox = &con->recvmbox;
    recv_timeout = con->recv_timeout;
    type = con->type;
    
    switch (type)
    {
        case CONN_UDP:
            if (MIP_TRUE == los_mip_conn_is_nonblocking(con))
            {
                ret = los_mip_mbox_fetch(recvmbox, (void **)&msgs, 1);
            }
            else
            {
                ret = los_mip_mbox_fetch(recvmbox, (void **)&msgs, recv_timeout);
            }
            if (MIP_OK != ret)
            {
                break;
            }
            con->rcvcnt--;
            if (NULL != from)
            {
                in.sin_family = AF_INET;
                in.sin_addr.s_addr = msgs->addr.addr;
                in.sin_port = msgs->port;
                /* copy address to up layer */
                memcpy(from, &in, sizeof(struct sockaddr_in));
            }
            if (NULL != fromlen)
            {
                *fromlen = sizeof(struct sockaddr_in);
            }
            
            copylen = len > msgs->p->len? msgs->p->len:len;
            memcpy(mem, msgs->p->payload, copylen);
            /* when udp, if you just recived part of the package, the rest will discard */
            netbuf_free(msgs->p);
            msgs->p = NULL;
            los_mip_delete_up_sktmsg(msgs);
            break;
        case CONN_TCP:
            copylen = los_mip_tcp_read(con, mem, len);
            break;
        default:
            break;
    }
    return copylen; 
}

/*****************************************************************************
 Function    : los_mip_bind
 Description : compatible socket interface. bind socket to a local address
 Input       : s @ socket's handle 
               name @ local address data
               namelen @ length of *name
 Output      : None
 Return      : ret @ 0 means bind ok, <0 means bind failed.
 *****************************************************************************/
int los_mip_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret = 0;
    struct sockaddr_in *tmp = NULL;
    struct mip_conn *con = NULL;
    
    if ((s < 0) || (NULL == name))
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -1;
    }
    tmp = (struct sockaddr_in *)name;
    /* todo: tcp udp need call the same function */
    switch (con->type)
    {
        case CONN_UDP:
            if (NULL != con->ctlb.udp)
            {
                /* call udp bind, this is just for ipv4, ipv6 will bugs */
                ret = los_mip_udp_bind(con->ctlb.udp, &tmp->sin_addr.s_addr, tmp->sin_port); 
            }
            break;
        case CONN_TCP:
            /* call tcp bind, this is just for ipv4, ipv6 will bugs */
            ret = los_mip_conn_bind(con, &tmp->sin_addr.s_addr, tmp->sin_port);
        default:
            break;
    }
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_bind
 Description : compatible socket interface. bind socket to a local address
 Input       : s @ socket's handle 
               name @ local address data
               namelen @ length of *name
 Output      : None
 Return      : ret @ 0 means bind ok, <0 means bind failed.
 *****************************************************************************/
int los_mip_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret = 0;
    struct sockaddr_in *tmp = NULL;
    struct mip_conn *con = NULL;
    
    if ((s < 0) || (NULL == name))
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -1;
    }
    tmp = (struct sockaddr_in *)name;
    ret = los_mip_do_connect(con, &tmp->sin_addr.s_addr, tmp->sin_port);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_bind
 Description : compatible socket interface. bind socket to a local address
 Input       : s @ socket's handle 
               name @ local address data
               namelen @ length of *name
 Output      : None
 Return      : ret @ 0 means bind ok, <0 means bind failed.
 *****************************************************************************/
int los_mip_listen(int s, int backlog)
{
    int ret = 0;
    struct mip_conn *con = NULL;
    if ((s < 0) || (backlog < 0))
    {
        return -MIP_ERR_PARAM;
    }
    
    if (backlog > SOMAXCONN)
    {
        backlog = SOMAXCONN;
    }
    if (backlog == 0)
    {
        backlog = SOMINCONN;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -1;
    }
    ret = los_mip_tcp_listen(con, backlog);
    return ret;
}

/*****************************************************************************
 Function    : los_mip_accept
 Description : compatible socket interface. 
 Input       : s @ socket's handle 
               addr @ remote address info
               addrlen @ length of *addr
 Output      : None
 Return      : fd @ socket handle that accepted.
 *****************************************************************************/
int los_mip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int fd = -1;
    int ret = 0;
    struct mip_conn *con = NULL;
    struct skt_acpt_msg *acptmsg = NULL;
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if ((NULL == con) || !(con->state & STAT_LISTEN))
    {
        return -1;
    }
    if (MIP_TRUE != los_mip_mbox_valid(&con->ctlb.tcp->acptmbox))
    {
        return -1;
    }
    con->state |= STAT_ACCEPT;
    los_mip_con_send_tcp_msg(con, TCP_ACCEPT);
    if (MIP_TRUE == los_mip_conn_is_nonblocking(con))
    {
        ret = los_mip_mbox_fetch(&con->ctlb.tcp->acptmbox, 
                                (void **)&acptmsg, 1);
    }
    else
    {
        
        ret = los_mip_mbox_fetch(&con->ctlb.tcp->acptmbox, 
                                (void **)&acptmsg, con->recv_timeout);
    }
    con->state &= (~STAT_ACCEPT);
    if ((MIP_OK == ret) && (acptmsg != NULL))
    {
        fd = acptmsg->con->socket;
        los_mip_inc_ref(acptmsg->con);
    }
    else
    {
        /* set errno EWOULDBLOCK or EAGAIN*/
    }
    return fd;
}

/*****************************************************************************
 Function    : scan_sockets
 Description : scan all sockets to find if there are some data comes up. 
 Input       : maxfdp1 @ max socket handle's value 
               rset_in @ read fd sets need to detect
               wset_in @ write fd sets need to detect
               exceptset @ exception fd sets need to detect
 Output      : rset_o @ read fd sets that data updated
               wset_o @ write fd sets that data updated
               excptset_o @ exception fd sets that data updated
 Return      : nselected @ total number of how many sockets's data changed.
 *****************************************************************************/
static int scan_sockets(int maxfdp1, 
                        fd_set *rset_in, 
                        fd_set *wset_in, 
                        fd_set *exceptset,
                        fd_set *rset_o, 
                        fd_set *wset_o, 
                        fd_set *excptset_o)
{
    int i = 0;
    int nselected = 0;
    struct mip_conn *con = NULL;
    fd_set lrset, lwset, lecptset;
    
    FD_ZERO(&lrset);
    FD_ZERO(&lwset);
    FD_ZERO(&lecptset);
    
    for (i = 0; i < maxfdp1; i++)
    {
        /* fd is not in the set, continue  */
        if (!(rset_in && FD_ISSET(i, rset_in)) 
            && !(wset_in && FD_ISSET(i, wset_in)) 
            && !(exceptset && FD_ISSET(i, exceptset))) 
        {
            continue;
        }
        con = los_mip_get_conn(i);
        if (NULL == con)
        {
            continue;
        }
        if (rset_in && FD_ISSET(i, rset_in) && (con->rcvcnt > 0)) 
        {
            FD_SET(i, &lrset);
            nselected++;
        }
        /* todo: write set and exception set, unimplemented yet. */
    }
    if (rset_o)
    {
        *rset_o = lrset;
    }
    if (wset_o)
    {
        *wset_o = lwset;
    }
    if (excptset_o)
    {
        *excptset_o = lecptset;
    }
    return nselected;
}

/*****************************************************************************
 Function    : los_mip_select
 Description : compatible socket interface.find if there are some data comes up. 
 Input       : maxfdp1 @ max socket handle's value 
               readset @ read fd sets need to detect
               writeset @ write fd sets need to detect
               exceptset @ exception fd sets need to detect
               timeout @ timeout value
 Output      : readset @ read fd sets that data updated
               writeset @ write fd sets that data updated
               exceptset @ exception fd that data updated
 Return      : fdready @ total number of how many sockets's data changed.
 *****************************************************************************/
int los_mip_select(int maxfdp1, 
                   fd_set *readset, 
                   fd_set *writeset, 
                   fd_set *exceptset,
                   struct timeval *timeout)
{
    u32_t tmptm = 0xffffffff;
    fd_set lrset, lwset, lecptset;
    int fdready = 0;
    int i = 0;
    mip_sem_t waitsem;
    /* 
        if timeout is NULL , it should block 
        until fds can read or write or error occured 
    */
    if (maxfdp1 < 0)
    {
        return MIP_OK;
    }
    
    FD_ZERO(&lrset);
    FD_ZERO(&lwset);
    FD_ZERO(&lecptset);
    
    fdready = scan_sockets(maxfdp1, readset, 
                           writeset, exceptset, 
                           &lrset, &lwset, &lecptset);
    if (fdready)
    {
        /* some fd have data, can be read */
        if (readset)
        {
            *readset = lrset;
        }
        if (writeset)
        {
            *writeset = lwset;
        }
        if (exceptset)
        {
            *exceptset = lecptset;
        }
    }
    else
    {
        if (timeout)
        {
            tmptm = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
            if (!tmptm)
            {
                tmptm = 0xffffffff;
            }
        }
        los_mip_sem_new(&waitsem, 1);
        los_mip_sem_wait(&waitsem, 0);
        for (i = 0; i <= (tmptm / MIP_SOCKET_SEL_MIN_TM); i++)
        {
            los_mip_sem_wait(&waitsem, MIP_SOCKET_SEL_MIN_TM);
            fdready = scan_sockets(maxfdp1, readset, writeset, 
                                   exceptset, &lrset, &lwset, &lecptset);
            if (fdready)
            {
                /* some fd have data, can be read */
                if (readset)
                {
                    *readset = lrset;
                }
                if (writeset)
                {
                    *writeset = lwset;
                }
                if (exceptset)
                {
                    *exceptset = lecptset;
                }
                break;
            }
        }
        los_mip_sem_free(&waitsem);
    }
    return fdready;
}

/*****************************************************************************
 Function    : los_mip_sendto
 Description : compatible socket interface.send out data to remote server.
 Input       : s @ socket's handle 
               data @ data buf's pointer
               size @ data length
               flags @ Specifies the type of message transmission
               to @ dest ip address info
               tolen @ length of *to.
 Output      : None
 Return      : ret @ length of data that already sended.
 *****************************************************************************/
int los_mip_sendto(int s, 
                   const void *data, 
                   size_t size, 
                   int flags,
                   const struct sockaddr *to, 
                   socklen_t tolen)
{
    int ret = 0;
    struct sockaddr_in *tmp = NULL;
    struct mip_conn *con = NULL;
    struct netbuf *pbuf = NULL;
    
    if ((NULL == data) || (NULL == to))
    {
        return -1;
    }
    pbuf = netbuf_alloc(NETBUF_TRANS, size);
    if (NULL == pbuf)
    {
        return -1;
    }
    memcpy(pbuf->payload, data, size);
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -1;
    }
    switch (con->type)
    {
        case CONN_UDP:
            if (NULL != con->ctlb.udp)
            {
                /* call udp bind, this is just for ipv4, ipv6 will bugs */
                tmp = (struct sockaddr_in *)to;
                con->ctlb.udp->remote_ip.addr = tmp->sin_addr.s_addr;
                con->ctlb.udp->rport = tmp->sin_port;
                con->ctlb.udp->ttl = MIP_IP_DEFAULT_TTL;
                con->ctlb.udp->tos = 0;
            }
            ret = los_mip_snd_sktmsg(con, pbuf); 
            if (ret == MIP_OK)
            {
                ret = size;
            }
            else
            {
                netbuf_free(pbuf);
            }
            break;
        default:
            break;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_ioctl
 Description : compatible socket interface.set socket options.now just 
               set nonblock model.
 Input       : s @ socket's handle 
               cmd @ the ioctl cmd 
               argp @ the cmd value's pointer
 Output      : None
 Return      : 0 means process ok, other failed.
 *****************************************************************************/
int los_mip_ioctl(int s, long cmd, void *argp)
{
    int val = 0;
    struct mip_conn *con = NULL;
    
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    switch (cmd)
    {
        case FIONBIO:
            if(argp)
            {
                val = *(int *)argp;
            }
            /* set connect block or nonblock */
            los_mip_set_nonblocking(con, val);
            break;
        default:
            break;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_fcntl
 Description : compatible socket interface.set socket options.now just 
               set nonblock model.
 Input       : s @ socket's handle 
               cmd @ the ioctl cmd 
               val @ the cmd value
 Output      : None
 Return      : 0 means process ok, other failed.
 *****************************************************************************/
int los_mip_fcntl(int s, int cmd, int val)
{
    struct mip_conn *con = NULL;
    
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    switch (cmd)
    {
        case F_SETFL:
            /* set connect block or nonblock */
            if ((val & O_NONBLOCK) == 0) 
            {
                los_mip_set_nonblocking(con, 0);
            }
            else
            {
                los_mip_set_nonblocking(con, O_NONBLOCK);
            }
            break;
        default:
            break;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_ioctlsocket
 Description : compatible socket interface.set socket options.now just 
               set nonblock model.
 Input       : s @ socket's handle 
               cmd @ the ioctl cmd 
               argp @ the cmd value's pointer
 Output      : None
 Return      : 0 means process ok, other failed.
 *****************************************************************************/
int los_mip_ioctlsocket(int s, long cmd, void *argp)
{
    return los_mip_ioctl(s, cmd, argp);
}

/*****************************************************************************
 Function    : los_mip_ioctlsocket
 Description : compatible socket interface.set socket options.
 Input       : s @ socket's handle 
               level @ specifies the protocol level 
               optname @  specifies a single option to set
               optval @ option data's pointer
               optlen @ option data's length
 Output      : None
 Return      : 0 means process ok, other failed.
 *****************************************************************************/
int los_mip_setsockopt(int s, int level, 
                       int optname, 
                       const void *optval, 
                       socklen_t optlen)
{
    int ret = MIP_OK;
    struct mip_conn *con = NULL;
    
    if ((s < 0) || (NULL == optval))
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    switch (level)
    {
        case SOL_SOCKET:
            switch (optname)
            {
                case SO_RCVTIMEO:
                    ret = los_mip_con_set_rcv_timeout(con, optval, optlen);
                    break;
                case SO_KEEPALIVE:
                    /* unimplement yet */
                default:
                    return -MIP_SOCKET_ERR;
            }
            break;
        case IPPROTO_TCP:
            switch (optname)
            {
                case TCP_NODELAY:
                    if (*(const int *)optval)
                    {
                        los_mip_tcp_nagle_disable(con);
                    }
                    else
                    {
                        los_mip_tcp_nagle_enable(con);
                    }
                    break;
                case TCP_KEEPALIVE:
                    /* unimplement yet */
                    break;
                case TCP_QUICKACK:
                    if (*(const int *)optval)
                    {
                        los_mip_tcp_quickack_enable(con);
                    }
                    else
                    {
                        los_mip_tcp_quickack_disable(con);
                    }
                    break;
                default:
                    return -MIP_SOCKET_ERR;
            }
            break;
        default:
            return -1;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_shutdown
 Description : compatible socket interface.set socket options.
 Input       : s @ socket's handle 
               how @ specifies the type of shutdown SHUT_RD SHUT_WR SHUT_RDWR 
 Output      : None
 Return      : 0 means process ok, other failed.
 *****************************************************************************/
int los_mip_shutdown(int s, int how)
{
    struct mip_conn *con = NULL;
    
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    los_mip_tcp_do_shutdown(con, how);
    return 0;
}
