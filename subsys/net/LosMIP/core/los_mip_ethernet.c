#include "los_mip_err.h"
#include "los_mip_ethernet.h"
#include "los_mip_ipv4.h"
#include "los_mip_arp.h"

int los_mip_eth_arp_output(struct netif *dev, struct netbuf *p, struct eth_mac *src, struct eth_mac *dst)
{
    struct eth_hdr *h = NULL;
    
    if (NULL == dev || NULL == p || NULL == src || NULL == dst)
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

int los_mip_eth_ip_output(struct netif *dev, struct netbuf *p, struct eth_mac *src, struct eth_mac *dst, enum mip_eth_types type)
{
    struct eth_hdr *h = NULL;
    
    if (NULL == dev || NULL == p || NULL == src || NULL == dst)
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


u16_t los_mip_get_ip_ver(struct netbuf *p)
{
    if (NULL == p)
    {
        return TYPE_MAX; 
    }
    return p->ipver;
}
int los_mip_set_ip_ver(struct netbuf *p,  u16_t ver)
{
    if (NULL == p)
    {
        return -MIP_ERR_PARAM; 
    }
    p->ipver = ver;
    return MIP_OK;
}

static int los_mip_is_broadcast_mac(struct eth_mac *mac)
{
    int i = 0;
    if (NULL == mac)
    {
        return MIP_FALSE;
    }
    for(i = 0; i < MIP_ETH_MAC_LEN; i++)
    {
        if (mac->hwaddr[i] != 0xff)
        {
            return MIP_FALSE;
        }
    }
    return MIP_TRUE;
}

static int los_mip_is_dst_to_us(struct netif *dev, struct eth_mac *mac)
{
    int i = 0;
    if (NULL == mac)
    {
        return MIP_FALSE;
    }
    for(i = 0; i < MIP_ETH_MAC_LEN; i++)
    {
        if (mac->hwaddr[i] != dev->hwaddr[i])
        {
            return MIP_FALSE;
        }
    }
    return MIP_TRUE;
}

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
    if (MIP_TRUE != los_mip_is_broadcast_mac(&h->dst)
        && MIP_TRUE != los_mip_is_dst_to_us(dev, &h->dst))
    {
        netbuf_free(p);
        return MIP_OK;
    }
    /* adjust payload to ip/arp/.... header */
    los_mip_jump_header(p, MIP_ETH_HDR_LEN);
    switch(type)
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
