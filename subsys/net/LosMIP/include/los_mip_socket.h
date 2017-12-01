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
#ifndef _LOS_MIP_SOCKET_H
#define _LOS_MIP_SOCKET_H

#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipaddr.h"
#include "los_mip_inet.h"

#include "los_mip_connect.h"

typedef u32_t socklen_t;


#define FD_SIZE    MIP_CONN_MAX
#define FD_SET(n, p)  ((p)->fd_bits[(n)/8] |=  (1 << ((n) & 7)))
#define FD_CLR(n, p)  ((p)->fd_bits[(n)/8] &= ~(1 << ((n) & 7)))
#define FD_ISSET(n,p) ((p)->fd_bits[(n)/8] &   (1 << ((n) & 7)))
#define FD_ZERO(p)    memset((void*)(p), 0, sizeof(*(p)))

typedef struct fd_set 
{
    unsigned char fd_bits [(FD_SIZE+7)/8];
} fd_set;


#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3


#define AF_INET 1

#define IPPROTO_IP      0
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#define SOL_SOCKET 0x800   /* options for socket level */

#define  SO_DEBUG       0x0001 /* Unimplemented: turn on debugging info recording */
#define  SO_ACCEPTCONN  0x0002 /* Unimplemented: socket has had listen() */
#define  SO_REUSEADDR   0x0004 /* Unimplemented: Allow local address reuse */
#define  SO_KEEPALIVE   0x0008 /* Unimplemented: keep connections alive */
#define  SO_DONTROUTE   0x0010 /* Unimplemented: Unimplemented: just use interface addresses */
#define  SO_BROADCAST   0x0020 /* Unimplemented: permit to send and to receive broadcast
                                  messages (see IP_SOF_BROADCAST option) */
#define  SO_USELOOPBACK 0x0040 /* Unimplemented: Unimplemented: bypass hardware when possible */
#define  SO_LINGER      0x0080 /* Unimplemented: linger on close if data present */
#define  SO_OOBINLINE   0x0100 /* Unimplemented: Unimplemented: leave received OOB data in line */
#define  SO_REUSEPORT   0x0200 /* Unimplemented: Unimplemented: allow local address & port reuse */

#define SO_DONTLINGER   ((int)(~SO_LINGER))

#define SHUT_RD    MIP_SHUT_RD      /* disables further receive operations */
#define SHUT_RDWR  MIP_SHUT_RDWR    /* disables further send and receive operations */
#define SHUT_WR    MIP_SHUT_WR      /* disables further send operations */

#define EPERM   1        /* Operation not permitted */

/*
    Additional options, not kept in so_options.
*/
#define SO_SNDBUF    0x1001    /* Unimplemented: send buffer size */
#define SO_RCVBUF    0x1002    /* Unimplemented: receive buffer size */
#define SO_SNDLOWAT  0x1003    /* Unimplemented: Unimplemented: send low-water mark */
#define SO_RCVLOWAT  0x1004    /* Unimplemented: Unimplemented: receive low-water mark */
#define SO_SNDTIMEO  0x1005    /* Unimplemented: send timeout */
#define SO_RCVTIMEO  0x1006    /* receive timeout */
#define SO_ERROR     0x1007    /* Unimplemented: get error status and clear */
#define SO_TYPE      0x1008    /* Unimplemented: get socket type */
#define SO_CONTIMEO  0x1009    /* Unimplemented: connect timeout */
#define SO_NO_CHECK  0x100a    /* Unimplemented: don't create UDP checksum */

#define FIONBIO     0x40000080UL
#define F_SETFL     1
#define O_NONBLOCK  1

#define SOMAXCONN   (MIP_CONN_MAX - 1)  /* default max connection in accept box */
#define SOMINCONN   1                   /* default min connection in accept box */

#define MIP_SOCKET_SEL_MIN_TM 5

int los_mip_socket(int domain, int type, int protocol);
int los_mip_bind(int s, const struct sockaddr *name, socklen_t namelen);
int los_mip_listen(int s, int backlog);
int los_mip_connect(int s, const struct sockaddr *name, socklen_t namelen);
int los_mip_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int los_mip_sendto(int s, const void *data, size_t size, int flags,
            const struct sockaddr *to, socklen_t tolen);
int los_mip_recv(int s, void *mem, size_t len, int flags);
int los_mip_recvfrom(int s, void *mem, size_t len, int flags,
        struct sockaddr *from, socklen_t *fromlen);
int los_mip_close(int s);
int los_mip_setsockopt(int s, int level, int optname, 
                       const void *optval, socklen_t optlen);
int los_mip_ioctlsocket(int s, long cmd, void *argp);
int los_mip_ioctl(int s, long cmd, void *argp);
int los_mip_fcntl(int s, int cmd, int val);
int los_mip_select(int maxfdp1, fd_set *readset, 
                   fd_set *writeset, fd_set *exceptset,
                   struct timeval *timeout);
int los_mip_write(int s, const void *data, size_t size);
int los_mip_read(int s, void *mem, size_t len);
#endif
