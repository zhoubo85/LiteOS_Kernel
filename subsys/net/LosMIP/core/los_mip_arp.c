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

#include "los_mip_config.h"
#include "los_mip_typed.h"
#include "los_mip_arp.h"
#include "los_mip_ipaddr.h"
#include "los_mip_osfunc.h"
#include "los_mip_ethernet.h" 
#include "los_mip_netbuf.h"
#include "los_mip_ipv4.h"

static struct arp_table g_arp_tabs[MIP_ARP_TAB_SIZE];
static struct eth_mac broardcastmac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static struct eth_mac zeromac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

/*****************************************************************************
 Function    : los_mip_update_arp_tab
 Description : update arp table's item
 Input       : ip  @ ip address of the remote network device
               mac @ mac address of the remote network device 
 Output      : None
 Return      : MIP_OK on success
 *****************************************************************************/
int los_mip_update_arp_tab(struct ipv4_addr ip, u8_t *mac) 
{
    int i = 0;
    u32_t tm = 0;
    int repos = 0;
    
    if (NULL == mac)
    {
        return -MIP_ERR_PARAM;
    }
    for (i = 0; i < MIP_ARP_TAB_SIZE; i++)
    {
        if ((g_arp_tabs[i].ip.addr == ip.addr) || (g_arp_tabs[i].ip.addr == MIP_IP_ANY))
        {
            g_arp_tabs[i].ip.addr = ip.addr;
            memcpy(g_arp_tabs[i].hw.hwaddr, mac, MIP_ETH_MAC_LEN);
            g_arp_tabs[i].tm = los_mip_cur_tm();
            break;
        }
        if ((tm == 0) || (tm > g_arp_tabs[i].tm))
        {
            repos = i;
            tm = g_arp_tabs[i].tm;
        }
    }
    /* not find and the arp tab is full, replace the oldest arp */
    if (i == MIP_ARP_TAB_SIZE)
    {
        g_arp_tabs[repos].ip.addr = ip.addr;
        memcpy(g_arp_tabs[repos].hw.hwaddr, mac, MIP_ETH_MAC_LEN);
        g_arp_tabs[repos].tm = los_mip_cur_tm();
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_find_arp_entry
 Description : find a mac's value by ip from arp talbe
 Input       : ip     @ ip address
 Output      : dstmac @ the mac's value of the ip
 Return      : MIP_OK on success, -MIP_ARP_NOT_FOUND on not found 
 *****************************************************************************/
int los_mip_find_arp_entry(struct ipv4_addr *ip, struct eth_mac **dstmac)
{
    int i = 0;

    for (i = 0; i < MIP_ARP_TAB_SIZE; i++)
    {
        if (g_arp_tabs[i].ip.addr == ip->addr)
        {
            if (NULL != dstmac)
            {
                *dstmac = &g_arp_tabs[i].hw;
            }
            //memcpy(dstmac, g_arp_tabs[i].hw.hwaddr, MIP_ETH_MAC_LEN);
            return MIP_OK;
        }
    }
    return -MIP_ARP_NOT_FOUND;
}    

/*****************************************************************************
 Function    : los_mip_arp_xmit
 Description : first find if the dstip in the arp table, if not, send arp request
               else get dstip's mac and send the package.
 Input       : dev @ net interface handle,which the pacage will send out
               p   @ up layer's pacage data
               dstip @ ip address of the package's destination.
 Output      : 
 Return      : MIP_OK on success, other value(< 0) on failed.pleas refer to 
               the macro defined.
 *****************************************************************************/
int los_mip_arp_xmit(struct netif *dev, struct netbuf *p, ip_addr_t *dstip)
{
    struct eth_mac *dstmac;
    ip_addr_t *reqip = NULL;
    int ret = MIP_OK;
    
    if ((NULL == dev) || (NULL == p) || (NULL == dstip))
    {
        return -MIP_ERR_PARAM;
    }
    
    if (dstip->addr == MIP_IP_BROADCAST)
    {
        dstmac = &broardcastmac;
    }
    else if (los_mip_is_multicast(dstip)) 
    {
        /* muticast not support yet, so we free the netbuf */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    else
    {
        /* find ip in arp table */
        if ((dstip->addr & dev->gw.addr) == (dev->ip_addr.addr & dev->gw.addr))
        {
            /* in the same net work */
            reqip = dstip;
        }
        else
        {
            reqip = &dev->gw;
        }
        if (dev->gw.addr == MIP_IP_ANY)
        {
            netbuf_free(p);
            return -MIP_ETH_ERR_PARAM;
        }
        if (MIP_OK != los_mip_find_arp_entry(reqip, &dstmac))
        {
            /* should send the package back ,and then send the pack */
            ret = netbuf_que_for_arp(p);
            if (ret != MIP_OK)
            {
                /* two many packages buffered wait for arp come */
                netbuf_free(p);
                return -MIP_ARP_SEND_FAILED;
            }
            /* gen arp request */
            return los_mip_arp_request(dev, reqip);
        }
        
    }
    /* call ethernet func to send package out */
    return los_mip_eth_ip_output(dev, p, (struct eth_mac *)&dev->hwaddr, dstmac, TYPE_IPV4);
}

/*****************************************************************************
 Function    : los_mip_arp_request
 Description : generate arp request package and send out it
 Input       : dev @ net interface handle,which the package will send out
               ipaddr @ dest ip address that need send to.
 Output      : 
 Return      : MIP_OK on success, other value(< 0) on failed.pleas refer to 
               the macro defined.
 *****************************************************************************/
int los_mip_arp_request(struct netif *dev, ip_addr_t *ipaddr)
{
    struct netbuf *p = NULL;
    struct mip_arp_hdr *arph = NULL;
    struct eth_hdr *ethh = NULL;
    
    if((NULL == dev) || (NULL == ipaddr))
    {
        return -MIP_ERR_PARAM;
    }
    p = netbuf_alloc(NETBUF_RAW, MIP_ARP_HEADER_LEN + MIP_ETH_HDR_LEN);
    if (NULL == p)
    {
        return -MIP_MPOOL_MALLOC_FAILED;
    }
    ethh = (struct eth_hdr *)p->payload;
    arph = (struct mip_arp_hdr *)((u8_t *)p->payload + MIP_ETH_HDR_LEN);
    
    memcpy(ethh->dst.hwaddr, broardcastmac.hwaddr, sizeof(broardcastmac));
    memcpy(ethh->src.hwaddr, dev->hwaddr, sizeof(broardcastmac));
    ethh->type = MIP_HTONS(TYPE_ARP);
      
    arph->hwtype = MIP_HTONS(TYPE_ETH);
    arph->proto = MIP_HTONS(TYPE_IPV4);
    arph->hwlen = MIP_ETH_HW_LEN;
    arph->protolen = sizeof(struct ipv4_addr);
    arph->opcode = MIP_HTONS(ARP_REQ);
    
    memcpy(arph->sndmac.hwaddr, dev->hwaddr, MIP_ETH_HW_LEN);
    memcpy(&arph->sndip, &dev->ip_addr, sizeof(struct ipv4_addr));
    memcpy(arph->dstmac.hwaddr, zeromac.hwaddr, MIP_ETH_HW_LEN);
    memcpy(&arph->dstip, ipaddr, sizeof(struct ipv4_addr));

    dev->hw_xmit(dev, p);
    netbuf_free(p);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_arp_for_us
 Description : judge if the arp package's dest is for us 
 Input       : inip @ dest ip
               dev @ net interface handle,which the pacage come from
 Output      : 
 Return      : MIP_TRUE package is for us. MIP_FALSE not for us
 *****************************************************************************/
static int los_mip_arp_for_us(struct netif *dev, u32_t inip)
{
    if (dev->ip_addr.addr == inip)
    {
        return MIP_TRUE;
    }
    return MIP_FALSE;
}

/*****************************************************************************
 Function    : los_mip_arp_input
 Description : deal with the arp packages recived from the net interface.
 Input       : dev @ net interface handle,which the pacage come from
               p @ the package that recieved.
 Output      : 
 Return      : MIP_OK process done.
 *****************************************************************************/
int los_mip_arp_input(struct netif *dev, struct netbuf *p)
{
    struct mip_arp_hdr *h = NULL;
    
    if ((NULL == dev) || (NULL == p))
    {
        return MIP_OK;
    }
    h = (struct mip_arp_hdr *)p->payload;
    if ((h->hwtype != MIP_HTONS(TYPE_ETH))
        || (h->proto != MIP_HTONS(TYPE_IPV4))
        || (h->protolen != sizeof(struct ipv4_addr)))
    {
        netbuf_free(p);
        return MIP_OK;
    }
    if (MIP_TRUE != los_mip_arp_for_us(dev, h->dstip.addr))
    {
        los_mpools_free(MPOOL_NETBUF, p);
        return MIP_OK;
    }
    
    switch (h->opcode)
    {
        case MIP_HTONS(ARP_REQ):
            /* gen arp data, call ethnet out func to send data */
            h->opcode = MIP_HTONS(ARP_REP);
            memcpy(&h->dstip, &h->sndip, sizeof(struct ipv4_addr));
            memcpy(&h->dstmac, &h->sndmac, sizeof(h->sndmac));
            los_mip_update_arp_tab(h->sndip, h->sndmac.hwaddr);
            
            memcpy(&h->sndip, &dev->ip_addr, sizeof(struct ipv4_addr));
            memcpy(&h->sndmac, &dev->hwaddr, sizeof(h->sndmac));
            los_mip_eth_arp_output(dev, p, &h->sndmac, &h->dstmac);
            break;
        case MIP_HTONS(ARP_REP):
            los_mip_update_arp_tab(h->sndip, h->sndmac.hwaddr);
            break;
        default:
            break;
    }
    
    netbuf_free(p);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_arp_tab_init
 Description : set arp table empty
 Input       : None
 Output      : None
 Return      : MIP_OK init process done.
 *****************************************************************************/
int los_mip_arp_tab_init(void)
{
    memset(g_arp_tabs, 0, MIP_ARP_TAB_SIZE*sizeof(struct arp_table));
    return MIP_OK;
}
