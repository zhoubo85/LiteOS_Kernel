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
 
#include "los_mip_err.h"
#include "los_mip_ethernet.h"
#include "los_mip_ipv4.h"
#include "los_mip_arp.h"

/*****************************************************************************
 Function    : los_mip_eth_arp_output
 Description : transmit arp package message
 Input       : dev @ ethnet dev's pointer
               p @ arp package buf pointer
               src @ source mac addr
               dst @ dest mac addr
 Output      : None
 Return      : MIP_OK send ok, other value means send failed.
 *****************************************************************************/
int los_mip_eth_arp_output(struct netif *dev, 
                           struct netbuf *p, 
                           struct eth_mac *src, 
                           struct eth_mac *dst)
{
    struct eth_hdr *h = NULL;
    
    if ((NULL == dev) || (NULL == p) || (NULL == src) || (NULL == dst))
    {
        return -MIP_ERR_PARAM;
    }
    los_mip_jump_header(p, -MIP_ETH_HDR_LEN);
    h = (struct eth_hdr *)p->payload;
    h->type = MIP_HTONS(TYPE_ARP);
    memcpy(&h->dst, dst, sizeof(struct eth_mac));
    memcpy(&h->src, src, sizeof(struct eth_mac));
    
    dev->hw_xmit(dev, p);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_eth_ip_output
 Description : transmit ip package message
 Input       : dev @ ethnet dev's pointer
               p @ arp package buf pointer
               src @ source mac addr
               dst @ dest mac addr
               type @ package type, like ipv4/ipv6 ...
 Output      : None
 Return      : MIP_OK send ok, other value means send failed.
 *****************************************************************************/
int los_mip_eth_ip_output(struct netif *dev, 
                          struct netbuf *p, 
                          struct eth_mac *src, 
                          struct eth_mac *dst, 
                          enum mip_eth_types type)
{
    struct eth_hdr *h = NULL;
    
    if ((NULL == dev) || (NULL == p) || (NULL == src) || (NULL == dst))
    {
        return -MIP_ERR_PARAM;
    }
    los_mip_jump_header(p, -MIP_ETH_HDR_LEN);
    h = (struct eth_hdr *)p->payload;
    h->type = MIP_HTONS(type);
    memcpy(&h->dst, dst, sizeof(struct eth_mac));
    memcpy(&h->src, src, sizeof(struct eth_mac));
    
    dev->hw_xmit(dev, p);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_get_ip_ver
 Description : get ip version 
 Input       : p @ package buf pointer
 Output      : None
 Return      : ip package version
 *****************************************************************************/
u16_t los_mip_get_ip_ver(struct netbuf *p)
{
    if (NULL == p)
    {
        return TYPE_MAX; 
    }
    return p->ipver;
}

/*****************************************************************************
 Function    : los_mip_set_ip_ver
 Description : set ip version 
 Input       : p @ package buf pointer
               ver @ ip package version
 Output      : None
 Return      : MIP_OK set version ok. other value set failed.
 *****************************************************************************/
int los_mip_set_ip_ver(struct netbuf *p,  u16_t ver)
{
    if (NULL == p)
    {
        return -MIP_ERR_PARAM; 
    }
    p->ipver = ver;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_is_broadcast_mac
 Description : judge if the mac is broardcast mac address 
 Input       : mac @ ethnet mac's pointer
 Output      : None
 Return      : MIP_TRUE mac is broadcast address, MIP_FALSE not broadcast addr
 *****************************************************************************/
static int los_mip_is_broadcast_mac(struct eth_mac *mac)
{
    int i = 0;
    if (NULL == mac)
    {
        return MIP_FALSE;
    }
    for (i = 0; i < MIP_ETH_MAC_LEN; i++)
    {
        if (mac->hwaddr[i] != 0xff)
        {
            return MIP_FALSE;
        }
    }
    return MIP_TRUE;
}

/*****************************************************************************
 Function    : los_mip_is_dst_to_us
 Description : judge if the data is to dev's data
 Input       : dev @ net dev's handle which the data come from
               mac @ ethnet mac's pointer
 Output      : None
 Return      : MIP_TRUE mac is broadcast address, MIP_FALSE not broadcast addr
 *****************************************************************************/
static int los_mip_is_dst_to_us(struct netif *dev, struct eth_mac *mac)
{
    int i = 0;
    
    if (NULL == mac)
    {
        return MIP_FALSE;
    }
    for (i = 0; i < MIP_ETH_MAC_LEN; i++)
    {
        if (mac->hwaddr[i] != dev->hwaddr[i])
        {
            return MIP_FALSE;
        }
    }
    return MIP_TRUE;
}

/*****************************************************************************
 Function    : los_mip_eth_input
 Description : ethnet input package process func
 Input       : dev @ net dev's handle which the data come from
               p @ the package's buf pointer
 Output      : None
 Return      : MIP_OK process ok, other process failed.
 *****************************************************************************/
int los_mip_eth_input(struct netbuf *p, struct netif *dev)
{
    struct eth_hdr *h = NULL;
    int needfree = 0;
    int ret = MIP_OK;
    u16_t type = 0;
    

    if (NULL == p)
    {
        return MIP_OK;
    }
    if (p->len <= MIP_ETH_HDR_LEN)
    {
        /* package data invalid, so discard it. */
        netbuf_free(p);
        return MIP_OK;
    }
    h = (struct eth_hdr *)p->payload;
    type = h->type;
    if ((MIP_TRUE != los_mip_is_broadcast_mac(&h->dst))
        && (MIP_TRUE != los_mip_is_dst_to_us(dev, &h->dst)))
    {
        netbuf_free(p);
        return MIP_OK;
    }
    /* adjust payload to ip/arp/.... header */
    los_mip_jump_header(p, MIP_ETH_HDR_LEN);
    switch (type)
    {
        case MIP_HTONS(TYPE_IPV4):
            /* record cur pacaget ip version */
            los_mip_set_ip_ver(p, (u16_t)TYPE_IPV4);
            ret = los_mip_ipv4_input(p, dev);
            break;
        case MIP_HTONS(TYPE_ARP):
            ret = los_mip_arp_input(dev, p);
            break;
        case MIP_HTONS(TYPE_IPV6):
            los_mip_set_ip_ver(p, (u16_t)TYPE_IPV6);
            needfree = 1;
            break;
        default:
            needfree = 1;
            break;
    }
    if (needfree)
    {
        netbuf_free(p);
    }
    return ret;
}
