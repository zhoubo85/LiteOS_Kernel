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
 
#ifndef _LOS_MIP_CONNECT_H
#define _LOS_MIP_CONNECT_H

#include "los_mip_config.h"
#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipaddr.h"
#include "los_mip_osfunc.h"
#include "los_mip_udp.h"
#include "los_mip_tcp.h"

#define MIP_CONN_NONBLOCK           0x01

#define MIP_CONN_MSG_RESEND_MAX     3
#define MIP_RANDOM_PORT_BASE_VALUE  49152

#define MIP_SOCKET_LOCKED           1
#define MIP_SOCKET_UNLOCKED         0

enum conn_type 
{
    CONN_INVALID     = 0,
    CONN_TCP         = 0x10,/* TCP IPv4 */ 
    CONN_UDP         = 0x20,/* UDP IPv4 */
    CONN_MAX
};


#define STAT_NONE       0x00 
#define STAT_LISTEN     0x01 /* TCP listen */ 
#define STAT_CONNECT    0x02 /* TCP/UDP connect */ 
#define STAT_RD         0x04 /* TCP/UDP read */ 
#define STAT_CLOSE      0x08 /* TCP/UDP Close */ 
#define STAT_ACCEPT     0x10 /* TCP accept */ 
#define STAT_MAX


struct skt_rcvd_msg
{
    struct skt_rcvd_msg *next;
    struct netbuf *p;
    ip_addr_t addr;
    u16_t port;
};

/*  netconn descriptor */
struct mip_conn 
{
    int socket;
    /* type of the netconn (TCP, UDP or RAW) */
    enum conn_type type;
    /* current state of the netconn */
    u8_t state;
    /* the protocol control block */
    union 
    {
        struct tcp_ctl *tcp;
        struct udp_ctl *udp;
    } ctlb;/* tcp udp control block */
    struct mip_conn *listen;/* connection's listen connect */
    
    mip_sem_t op_finished;/* snd data op finished */ 
    mip_mbox_t recvmbox;
    s32_t send_timeout;
    int recv_timeout;
    short last_err;
    u8_t flags;
    u8_t rcvcnt;
    u8_t ref;           /* connection reference count */
    u16_t backlog;       /* record the backlog and un-accept count value */
    struct mip_msg *cur_msg;
    struct skt_rcvd_msg *rcvdata;
};

struct skt_msg_dat
{
    struct mip_conn *con;
    struct netbuf *p;
    int err;
    int retry;/* when arp not find , need resend msg, and */
    mip_sem_t* op_finished;
};

struct skt_msg
{
    int (* sktsndfn)(struct skt_msg_dat *msgdat);
    struct skt_msg_dat data;/* args for sktfn() */ 
};

struct skt_del_msg
{
    int (* sktdelfn)(struct mip_conn *con);
    struct mip_conn *con;
};

/* tcp accept messages */ 
struct skt_acpt_msg
{
    struct mip_conn *con;
};

enum tcp_msg_type 
{
    TCP_CONN    = 1, /* start connect remote server */
    TCP_CLOSE   = 2, /* user call close funtion */
    TCP_SEND    = 3, /* user call send funtion, used for send immediately */
    TCP_READ    = 4, /* user call read funtion, used for read immediately */
    TCP_ACCEPT  = 5, /* start accept socket to upper layer */
    TCP_NONE
};
/* tcp accept messages */ 
struct skt_tcp_msg
{
    enum tcp_msg_type type;
    struct mip_conn *con;
};

int los_mip_port_is_used(u16_t port);
int los_mip_add_port_to_used_list(u16_t port);
int los_mip_remove_port_from_used_list(u16_t port);
u16_t los_mip_generate_port(void);
int  los_mip_conn_snd(struct skt_msg_dat *data);
int los_mip_inc_ref(struct mip_conn *con);
int los_mip_dec_ref(struct mip_conn *con);
struct mip_conn *los_mip_new_conn(enum conn_type type);
int los_mip_delete_conn(struct mip_conn *con);
struct mip_conn *los_mip_get_conn(int socket);
int los_mip_snd_sktmsg(struct mip_conn *con, struct netbuf *buf);
struct mip_conn *los_mip_walk_con(s8_t *finished);
struct skt_rcvd_msg *los_mip_new_up_sktmsg(struct netbuf *buf, 
                                           ip_addr_t *to, 
                                            u16_t port);
int los_mip_delete_up_sktmsg(struct skt_rcvd_msg *msg);
int los_mip_snd_msg_to_con(struct mip_conn *con, void *msg);
int los_mip_add_rcv_msg_to_tail(struct mip_conn *con, 
                                struct skt_rcvd_msg *msg);
int los_mip_con_set_rcv_timeout(struct mip_conn *con, 
                                const void *tm, u32_t len);
int los_mip_set_nonblocking(struct mip_conn *con, int val);
int los_mip_conn_is_nonblocking(struct mip_conn *con);
int los_mip_init_sockets(void);
int los_mip_lock_socket(int socket);
int los_mip_unlock_socket(int socket);
struct skt_del_msg *los_mip_new_delmsg(struct mip_conn *con);
int los_mip_snd_delmsg(struct mip_conn *con);
int los_mip_udp_bind(struct udp_ctl *udpctl, 
                     u32_t *dst_ip, u16_t dst_port);
int los_mip_conn_bind(struct mip_conn *conn, u32_t *dst_ip, 
                      u16_t dst_port);
int los_mip_tcp_listen(struct mip_conn *conn, int backlog);
int los_mip_do_connect(struct mip_conn *conn, 
                       u32_t *dst_ip, u16_t dst_port);
struct mip_conn * los_mip_tcp_get_conn(struct netbuf *buf, 
                                       ip_addr_t *src, 
                                       ip_addr_t *dst);
struct mip_conn *los_mip_clone_conn(struct mip_conn *tcpcon, 
                                    ip_addr_t dst,
                                    ip_addr_t rip, u16_t rport);
int los_mip_con_send_tcp_msg(struct mip_conn *conn, 
                             enum tcp_msg_type type);
struct skt_acpt_msg *los_mip_new_acptmsg(struct mip_conn *con);
int los_mip_snd_acptmsg_to_listener(struct mip_conn *listener, 
                                    struct skt_acpt_msg *msg);
int los_mip_tcp_close(struct mip_conn *conn);
int los_mip_tcp_read(struct mip_conn *conn, void *mem, size_t len);
int los_mip_tcp_write(struct mip_conn *conn, const void *mem, 
                      size_t len);
int los_mip_tcp_nagle_enable(struct mip_conn *conn);
int los_mip_tcp_nagle_disable(struct mip_conn *conn);
int los_mip_tcp_do_shutdown(struct mip_conn *conn, int how);
struct mip_conn * los_mip_tcp_get_unaccept_conn(struct mip_conn *listen);
#endif
