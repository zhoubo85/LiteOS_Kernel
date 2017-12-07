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
 
#ifndef _LOS_MIP_TCP_H
#define _LOS_MIP_TCP_H

#include "los_mip_typed.h"
#include "los_mip_config.h"
#include "los_mip_netif.h"
#include "los_mip_ipv4.h"
#include "los_mip_ipaddr.h"
#include "los_mip_netbuf.h"
#include "los_mip_connect.h"

#define MIP_TCP_HDR_LEN 20
#define MIP_TCP_OPT_MSS_LEN 4

/* mip tcp flags */
#define MIP_TCP_FIN 0x01
#define MIP_TCP_SYN 0x02
#define MIP_TCP_RST 0x04
#define MIP_TCP_PSH 0x08
#define MIP_TCP_ACK 0x10
#define MIP_TCP_URG 0x20
#define MIP_TCP_CTL 0x3f

#define MIP_TCP_OPT_END 0x00 /* stop option */
#define MIP_TCP_OPT_NON 0x01 /* no operate option */
#define MIP_TCP_OPT_MSS 0x02 /* max segment size option */
#define MIP_TCP_OPT_SCA 0x03 /* scale option */
#define MIP_TCP_OPT_UD4 0x04 /* unimplement yet */
#define MIP_TCP_OPT_UD5 0x05 /* unimplement yet */
#define MIP_TCP_OPT_UD6 0x06 /* unimplement yet */
#define MIP_TCP_OPT_UD7 0x07 /* unimplement yet */ 
#define MIP_TCP_OPT_TIM 0x08 /* time stamp option */

#define MIP_TCP_OPT_MSS_LEN 4

#define MIP_TCP_FLAGS_ACK       MIP_TCP_ACK
#define MIP_TCP_FLAGS_SYN_ACK   (MIP_TCP_ACK | MIP_TCP_SYN)
#define MIP_TCP_FLAGS_FIN_ACK   (MIP_TCP_ACK | MIP_TCP_FIN)
#define MIP_TCP_FLAGS_SYN       MIP_TCP_SYN
#define MIP_TCP_FLAGS_PSH_ACK   (MIP_TCP_ACK | MIP_TCP_PSH)

#define TCP_TIMER_KEEPALIVE 0x01 /* tcp keep alive timer working */
#define TCP_TIMER_PERSIST   0x02 /* tcp persist timer working */
#define TCP_TIMER_FIN_WAIT2 0x04 /* tcp fin wait 2 state timer working */
#define TCP_TIMER_2MSL      0x08 /* tcp timed wait state timer working */
#define TCP_TIMER_RETRANS   0x10 /* tcp retransmit timer work */
#define TCP_TIMER_INTERNAL  0x80 /* tcp socket base timer, 200 ms timer is working */

#define MIP_SHUT_RD    0x01 /* disables further receive operations */
#define MIP_SHUT_RDWR  0x11 /* disables further send and receive operations */
#define MIP_SHUT_WR    0x10 /* disables further send operations */

enum tcp_tmr_type
{
    TCP_TMR_N = -1,
    TCP_TMR_K = 0,
    TCP_TMR_P,
    TCP_TMR_F,
    TCP_TMR_2MSL,
    TCP_TMR_REXMIT,
    TCP_TMR_I,
    TCP_TMR_MAX
};

enum tcp_state 
{
    TCP_INITIAL     = -1,
    TCP_CLOSED      = 0,
    TCP_LISTEN      = 1,
    TCP_SYN_SENT    = 2,
    TCP_SYN_RCVD    = 3,
    TCP_ESTABLISHED = 4,
    TCP_FIN_WAIT_1  = 5,
    TCP_FIN_WAIT_2  = 6,
    TCP_CLOSE_WAIT  = 7,
    TCP_CLOSING     = 8,
    TCP_LAST_ACK    = 9,
    TCP_TIME_WAIT   = 10
};

PACK_STRUCT_BEGIN
struct tcp_hdr 
{
    PACK_STRUCT_FIELD(u16_t sport);     /* source port */
    PACK_STRUCT_FIELD(u16_t dport);     /* dest port */
    PACK_STRUCT_FIELD(u32_t seq);       /* sequence number */
    PACK_STRUCT_FIELD(u32_t ack);       /* ack number */
    PACK_STRUCT_FIELD(u16_t hl_r_flg);  /* header length, reserved and flags*/
    PACK_STRUCT_FIELD(u16_t wndsize);   /* windown size */
    PACK_STRUCT_FIELD(u16_t chksum);    /* check sum */
    PACK_STRUCT_FIELD(u16_t urgp);      /* urgent pointer */
};
PACK_STRUCT_END

#define TCP_NODELAY     0x01 /* don't delay send */
#define TCP_KEEPALIVE   0x02 /* need send keep alive message while no data send */
#define TCP_RXCLOSED    0x04 /* tcp rx closed by shutdown */
#define TCP_TXCLOSED    0x08 /* tcp tx closed by shutdown */
//#define TCP_INTIMOUT    0x80 /* tcp internal timer timeout */
#define MIP_TCP_FLAGS   0x3fU

#define MIP_TCP_HTONS(x) MIP_HTONS(x)
#define MIP_TCP_HTONL(x) MIP_HTONL(x)
#define MIP_TCP_NTOHS(x) MIP_NTOHS(x)
#define MIP_TCP_NTOHL(x) MIP_NTOHL(x)
#define MIP_TCPH_HDRLEN_FLAGS_SET(phdr, len, flags) (phdr)->hl_r_flg = MIP_HTONS(((len) << 12) | (flags))
#define MIP_TCPH_SET_FLAG(phdr, flags ) (phdr)->hl_r_flg = ((phdr)->hl_r_flg | MIP_HTONS(flags))
#define MIP_TCPH_HDRLEN(phdr) ((u16_t)(MIP_HTONS((phdr)->hl_r_flg) >> 12))
#define MIP_TCPH_FLAGS(phdr)  ((u16_t)(MIP_NTOHS((phdr)->hl_r_flg) & MIP_TCP_FLAGS))
#define MIP_TCPLEN(seg) ((seg)->len + (((MIP_TCPH_FLAGS((seg)->tcphdr) & (MIP_TCP_FIN | MIP_TCP_SYN)) != 0) ? 1U : 0U))


struct mip_tcp_seg
{
    struct mip_tcp_seg *next;       /* used when putting segments on a queue */
    struct netbuf *p;               /* buffer containing data + TCP header */
    u32_t  sndtm;                   /* record the time that the segement first send */
    u8_t  rexmitcnt;                /* segment rexmit count */
    u8_t  rearpcnt;                 /* segment rexmit for arp failed count */
    u16_t len;                      /* the TCP length of this segment */
    struct tcp_hdr *tcphdr;     /* the TCP header */
};

//struct tcp_ctl;

struct tcp_ctl
{
    ip_addr_t       localip;
    ip_addr_t       remote_ip; 
    u16_t           lport;      /* local port */
    u16_t           rport;      /* remote port */
    enum tcp_state  state;      /* tcp connection state */
    mip_mbox_t      acptmbox;   /* used for socket accept , its size if 1, 
                                   means every time get one connection come  */
    mip_timer_t     tmr;        /* tcp internel timer, used for calc all kind of timeout */
    u32_t           rto;        /* retransmit timeout value,  */
    u32_t           srtt;        /* current package sRTT  value  */
    u32_t           rttvar;     /* RTT average value used for estimate rto */
    
    u8_t            flag;       /* tcp flags , like nodelay .... */
    u8_t            rtocnt;     /* count of retransmit */
    u8_t            tmrmask;    /* indicate which timer is working */
    u16_t           lastseqlen; /* the last segment that reviced payload length */
    u16_t           persist;    /* persist timer count */
    u16_t           keepalive;  /* keep alive timer count */
    u16_t           finwait2;   /* fin wait2 timer count */
    u16_t           twomsl;     /* 2MSL timer count */
    u16_t           swnd;       /* sliding window */
    u16_t           cwnd;       /* congestion window */
    u16_t           unsndsize;  /* un-send data's length */
    u16_t           rcvbuflen;  /* revice buf size */
    
    u16_t           mss;        /* maximum segment size of remote endpoint */
    u16_t           localmss;   /* maximum segment size of remote endpoint */
    u32_t           isn;        /* Initial Sequence Number */
    u32_t           rcv_nxt;    /* the sequence number that we expect to receive next. */
    u32_t           snd_nxt;    /* next new seqno to be sent */
    u32_t           lastack;    /* highest acknowledged seqno. */
    struct netbuf   *unsend;    /* the unsend tcp segment list*/
    struct mip_tcp_seg *unacked; /* the unacked tcp segment list*/
    struct mip_tcp_seg *recievd; /* the recieved tcp segment list */
};

#define MIP_TCP_ACPTBOX_SIZE    1
#define MIP_TCP_MAX_RETRANS     10      /* default retransmit times */
#define MIP_TCP_RTO_MIN         200     /* 200 ms */
#define MIP_TCP_RTO_MAX         120000  /* 120 seconds */

//int los_mip_tcp_send_syn(struct netif *dev, struct mip_conn *con);
int los_mip_tcp_process_upper_msg(void* msg);
int los_mip_tcp_input(struct netbuf *p, struct netif *dev, 
                      ip_addr_t *src, ip_addr_t *dst);
u32_t los_mip_tcp_gen_isn(void);
int los_mip_tcp_delte_seg(struct mip_tcp_seg *seg);
void los_mip_tcp_timer_callback(u32_t uwPar);

#endif
