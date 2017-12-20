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

#include "los_mip_config.h"
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* store the netbuf's pointer which need wait arp done first */
static u32_t g_wait_arp_array[LOS_MIP_MAX_WAIT_CON] = {0};

/*****************************************************************************
 Function    : los_mip_jump_header
 Description : skip or add the header buf of the package
 Input       : p @ the package buf pointer
               size @ the header size(for input) or -size(for output)
Output      : None
 Return      : MIP_OK jump header process ok. other value means error happended
 *****************************************************************************/
int los_mip_jump_header(struct netbuf *p, s16_t size)
{
    if (NULL == p)
    {
        return MIP_OK;
    }
    if (p->len <= size)
    {
        return -MIP_MPOOL_JUMP_LEN_ERR;
    }
    if (((u8_t *)p->payload + size) < ((u8_t *)p + sizeof(struct netbuf)))
    {
         return -MIP_MPOOL_JUMP_LEN_ERR;
    }
    p->len -= size;
    p->payload = (u8_t *)p->payload + size;
    return MIP_OK;
}

/*****************************************************************************
 Function    : netbuf_alloc
 Description : malloc a buf for network package rom mempool.
 Input       : layer @ tcp/ip layer , for example NETBUF_TRANS is udp or tcp
               layer, NETBUF_IP is ip layer
               length @ buffer length
 Output      : None
 Return      : p @ netbuf's pointer
 *****************************************************************************/
struct netbuf *netbuf_alloc(nebubuf_layer layer, u16_t length)
{
    struct netbuf *p = NULL;
    int offset = 0;
    
    switch (layer)
    {
        case NETBUF_TRANS:
            offset = NETBUF_RESERVE_LEN + NETBUF_LINK_HLEN + NETBUF_IP_HLEN + NETBUF_TRANS_HLEN;
            break;
        case NETBUF_IP:
            offset = NETBUF_RESERVE_LEN + NETBUF_LINK_HLEN + NETBUF_IP_HLEN;
            break;
        case NETBUF_RAW:
            offset = 0;
            break; 
        default:
            return NULL;
    }
    p = (struct netbuf *)los_mpools_malloc(MPOOL_NETBUF, offset + length + sizeof(struct netbuf));
    if (NULL != p)
    {
        memset(p, 0, offset + length + sizeof(struct netbuf));
        p->len = length;
        p->next = NULL;
        p->payload = (u8_t *)p + sizeof(struct netbuf) + offset;
        p->ref = 1;
    }
    return p;
}

/*****************************************************************************
 Function    : netbuf_free
 Description : free a netbuf
 Input       : p @ netbuf that will free
 Output      : None
 Return      : MIP_OK free success, other value means free failed.
 *****************************************************************************/
int netbuf_free(struct netbuf *p)
{
    int ret = 0;
    
    if (NULL == p)
    {
        return MIP_OK;
    }
    ret = los_mpools_free(MPOOL_NETBUF, p);
    return ret;
}

/*****************************************************************************
 Function    : netbuf_que_for_arp
 Description : stored the package's pointer which need wait arp request first
 Input       : p @ netbuf that need wait arp request
 Output      : None
 Return      : MIP_OK store ok, other valude means error happended.
 *****************************************************************************/
int netbuf_que_for_arp(struct netbuf *p)
{
    int i = 0;
    int nqued = 0;
    int pos = -1;

    if (NULL == p)
    {
        return MIP_OK;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i++)
    {
        if (g_wait_arp_array[i] == (u32_t)(p))
        {
            nqued = 1;
            return MIP_OK;
        }
        if (g_wait_arp_array[i] == 0x0 && pos == -1)
        {
            pos = i;
        }
    }
    
    if (!nqued && pos != -1)
    {
        g_wait_arp_array[pos] = (u32_t)p;
    }
    else
    {
        return -MIP_ARP_QUE_FULL;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : netbuf_get_if_queued
 Description : get if a package is buffered for waitting arp requet done.
 Input       : p @ netbuf 's pointer
 Output      : None
 Return      : MIP_TRUE means netbuf is stored in queue. MIP_FALSE not stored.
 *****************************************************************************/
int netbuf_get_if_queued(struct netbuf *p)
{
    int i = 0;
    
    if (NULL == p)
    {
        return MIP_FALSE;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i++)
    {
        if (g_wait_arp_array[i] == (u32_t)(p))
        {
            return MIP_TRUE;
        }
    }
    return MIP_FALSE;
}

/*****************************************************************************
 Function    : netbuf_remove_queued
 Description : remove the stored package's pointer from the queue
 Input       : p @ the package's pointer that need to remove from queue.
 Output      : None
 Return      : MIP_OK remove ok.
 *****************************************************************************/
int netbuf_remove_queued(struct netbuf *p)
{
    int i = 0;
    
    if (NULL == p)
    {
        return MIP_OK;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i++)
    {
        if (g_wait_arp_array[i] == (u32_t)(p))
        {
            g_wait_arp_array[i] = 0x0;
            return MIP_OK;
        }
    }
    return MIP_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
