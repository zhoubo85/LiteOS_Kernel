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

int los_mip_update_arp_tab(struct ip4_addr ip, u8_t *mac) 
{
    int i = 0;
    u32_t tm = 0;
    int repos = 0;
    for(i = 0; i < MIP_ARP_TAB_SIZE; i++)
    {
        if (g_arp_tabs[i].ip.addr == ip.addr || g_arp_tabs[i].ip.addr == MIP_IP_ANY)
        {
            g_arp_tabs[i].ip.addr = ip.addr;
            memcpy(g_arp_tabs[i].hw.hwaddr, mac, MIP_ETH_MAC_LEN);
            g_arp_tabs[i].tm = los_mip_cur_tm();
            break;
        }
        if (tm == 0 || tm > g_arp_tabs[i].tm)
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

int los_mip_find_arp_entry(struct ip4_addr *ip, struct eth_mac **dstmac)
{
    int i = 0;

    for(i = 0; i < MIP_ARP_TAB_SIZE; i++)
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

int los_mip_arp_xmit(struct netif *dev, struct netbuf *p, ip_addr_t *dstip)
{
    struct eth_mac *dstmac;
    ip_addr_t *reqip = NULL;
    int ret = MIP_OK;
    if(NULL == dev || NULL == p || NULL == dstip)
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

/* 
    while send data to dest ip ,first should find mac in arp table
    if not found, make a arp request
*/
int los_mip_arp_request(struct netif *dev, ip_addr_t *ipaddr)
{
    struct netbuf *p = NULL;
    struct mip_arp_hdr *arph = NULL;
    struct eth_hdr *ethh = NULL;
    
    if(NULL == dev || NULL == ipaddr)
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
    arph->protolen = sizeof(struct ip4_addr);
    arph->opcode = MIP_HTONS(ARP_REQ);
    
    memcpy(arph->sndmac.hwaddr, dev->hwaddr, MIP_ETH_HW_LEN);
    memcpy(&arph->sndip, &dev->ip_addr, sizeof(struct ip4_addr));
    memcpy(arph->dstmac.hwaddr, zeromac.hwaddr, MIP_ETH_HW_LEN);
    memcpy(&arph->dstip, ipaddr, sizeof(struct ip4_addr));

    dev->hw_xmit(dev, p);
    netbuf_free(p);
    return MIP_OK;
}

static int los_mip_arp_for_us(struct netif *dev, u32_t inip)
{
    if (dev->ip_addr.addr == inip)
    {
        return MIP_TRUE;
    }
    return MIP_FALSE;
}

int los_mip_arp_input(struct netif *dev, struct netbuf *p)
{
    struct mip_arp_hdr *h = NULL;
    if (NULL == dev || NULL == p)
    {
        return MIP_OK;
    }
    h = (struct mip_arp_hdr *)p->payload;
    if(h->hwtype != MIP_HTONS(TYPE_ETH)
        || h->proto != MIP_HTONS(TYPE_IPV4)
        || h->protolen != sizeof(struct ip4_addr))
    {
        netbuf_free(p);
        return MIP_OK;
    }
    if (MIP_TRUE != los_mip_arp_for_us(dev, h->dstip.addr))
    {
        los_mpools_free(MPOOL_NETBUF, p);
        return MIP_OK;
    }
    switch(h->opcode)
    {
        case MIP_HTONS(ARP_REQ):
            //gen arp data, call ethnet out func to send data
            h->opcode = MIP_HTONS(ARP_REP);
            memcpy(&h->dstip, &h->sndip, sizeof(struct ip4_addr));
            memcpy(&h->dstmac, &h->sndmac, sizeof(h->sndmac));
            los_mip_update_arp_tab(h->sndip, h->sndmac.hwaddr);
            
            memcpy(&h->sndip, &dev->ip_addr, sizeof(struct ip4_addr));
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

int los_mip_arp_tab_init(void)
{
    memset(g_arp_tabs, 0, MIP_ARP_TAB_SIZE*sizeof(struct arp_table));
    return MIP_OK;
}
