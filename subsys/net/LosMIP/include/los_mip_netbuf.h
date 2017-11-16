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
 
#ifndef _LOS_MIP_NETPBUF_H
#define _LOS_MIP_NETPBUF_H

#include "los_mip_typed.h"

/* network packet buffer struct */
struct netbuf 
{
    /* next pbuf in singly linked pbuf chain */
    struct netbuf *next;

    /* pointer to the actual data in the buffer */
    void *payload;

    /* length of this buffer */
    u16_t len;

    /* misc flags */
    u8_t flags;
    u16_t ipver;/* ip package version */
    /*
        the reference count always equals the number of pointers
        that refer to this pbuf. This can be pointers from an application,
        the stack itself, or pbuf->next pointers from a chain.
   */
    u16_t ref;
};

typedef enum {
    NETBUF_TRANS,   /* transport layer buf(tcp,udp ... ) */
    NETBUF_IP,      /* ip layer buf */
    NETBUF_LINK,    /* link layer buf(add ethernet header ...) */
    NETBUF_RAW
} nebubuf_layer;

#define NETBUF_TRANS_HLEN 20
#define NETBUF_IP_HLEN 20
#define NETBUF_LINK_HLEN 14
#define NETBUF_RESERVE_LEN 32


int los_mip_jump_header(struct netbuf *p, s16_t size);
struct netbuf *netbuf_alloc(nebubuf_layer layer, u16_t length);
int netbuf_free(struct netbuf *p);
int netbuf_que_for_arp(struct netbuf *p);
int netbuf_get_if_queued(struct netbuf *p);
int netbuf_remove_queued(struct netbuf *p);

#endif
