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

int los_mip_socket(int family, int type, int protocol)
{
    int socket = -1;
    struct mip_conn *con = NULL;
    switch(family)
    {
        case AF_INET:
            break;
        default:
            return -MIP_SOCKET_NOT_SUPPORTED;
    }
    switch(type)
    {
        case SOCK_DGRAM:
            if (IPPROTO_UDP == protocol || 0 == protocol)
            {
                con = los_mip_new_conn(CONN_UDP);
            }
            break;
        case SOCK_STREAM:
            if (IPPROTO_TCP == protocol || 0 == protocol)
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

int los_mip_close(int s)
{
    int ret = MIP_OK;
    struct mip_conn *con = NULL;
    if (s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return MIP_OK;
    }
    los_mip_lock_socket(s);
    ret = los_mip_snd_delmsg(con);
    
    return ret;
}

int los_mip_read(int s, void *mem, size_t len)
{
    int ret = 0;
    if (NULL == mem || len <= 0 || s < 0)
    {
        return -MIP_ERR_PARAM;
    }
    ret = los_mip_recvfrom(s, mem, len, 0, NULL, NULL);
    
    return ret;
}

int los_mip_write(int s, const void *data, size_t size)
{
	struct mip_conn *con = NULL;
    if (NULL == data || size <= 0 || s < 0)
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
    return MIP_OK;
}

int los_mip_recv(int s, void *mem, size_t len, int flags)
{
    int ret = MIP_OK;
    if (s < 0 || NULL == mem)
    {
        return -MIP_ERR_PARAM;
    }
    ret = los_mip_recvfrom(s, mem, len, flags, NULL, NULL);
    return ret;
}

int los_mip_recvfrom(int s, void *mem, size_t len, int flags,
        struct sockaddr *from, socklen_t *fromlen) 
{
    struct mip_conn *con = NULL;
    struct skt_rcvd_msg *msgs = NULL;
    int ret = MIP_OK;
    struct sockaddr_in in;
    size_t copylen = 0;
    mip_mbox_t *recvmbox = NULL;
    int recv_timeout = 0;
    enum conn_type type = CONN_MAX;
    
    
    if (s < 0 || NULL == mem)
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


    switch(type)
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
            break;
        default:
            break;
    }
    return copylen; 
}

int los_mip_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret = 0;
    struct sockaddr_in *tmp = NULL;
    struct mip_conn *con = NULL;
    if (s < 0 || NULL == name)
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -1;
    }
    switch(con->type)
    {
        case CONN_UDP:
            if (NULL != con->ctlb.udp)
            {
                /* call udp bind, this is just for ipv4, ipv6 will bugs */
                tmp = (struct sockaddr_in *)name;
                los_mip_udp_bind(con->ctlb.udp, &tmp->sin_addr.s_addr, tmp->sin_port); 
            }
            break;
        default:
            break;
    }
    
    return ret;
}

int los_mip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return 0;
}

static int scan_sockets(int maxfdp1, fd_set *rset_in, fd_set *wset_in, fd_set *exceptset,
                        fd_set *rset_o, fd_set *wset_o, fd_set *excptset_o)
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
        if (!(rset_in && FD_ISSET(i, rset_in)) &&
            !(wset_in && FD_ISSET(i, wset_in)) &&
            !(exceptset && FD_ISSET(i, exceptset))) 
        {
            continue;
        }
        con = los_mip_get_conn(i);
        if (NULL == con)
        {
            continue;
        }
        if (rset_in && FD_ISSET(i, rset_in) && con->rcvcnt > 0) 
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

int los_mip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
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
    
    fdready = scan_sockets(maxfdp1, readset, writeset, exceptset, &lrset, &lwset, &lecptset);
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
        for(i = 0; i <= tmptm / MIP_SOCKET_SEL_MIN_TM; i++)
        {
            los_mip_sem_wait(&waitsem, MIP_SOCKET_SEL_MIN_TM);
            fdready = scan_sockets(maxfdp1, readset, writeset, exceptset, &lrset, &lwset, &lecptset);
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

int los_mip_sendto(int s, const void *data, size_t size, int flags,
            const struct sockaddr *to, socklen_t tolen)
{
    int ret = 0;
    struct sockaddr_in *tmp = NULL;
    struct mip_conn *con = NULL;
    struct netbuf *pbuf = NULL;
    if (NULL == data || NULL == to)
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
    switch(con->type)
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
    switch(cmd)
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
    switch(cmd)
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

int los_mip_ioctlsocket(int s, long cmd, void *argp)
{
    return los_mip_ioctl(s, cmd, argp);
}

int los_mip_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    int ret = MIP_OK;
    struct mip_conn *con = NULL;
    if (s < 0 || NULL == optval)
    {
        return -MIP_ERR_PARAM;
    }
    con = los_mip_get_conn(s);
    if (NULL == con)
    {
        return -MIP_ERR_PARAM;
    }
    switch(level)
    {
        case SOL_SOCKET:
            switch(optname)
            {
                case SO_RCVTIMEO:
                    ret = los_mip_con_set_rcv_timeout(con, optval, optlen);
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
