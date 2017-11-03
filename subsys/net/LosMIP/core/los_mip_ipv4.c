#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"
#include "los_mip_ethernet.h"
#include "los_mip_ipv4.h"
#include "los_mip_icmp.h"
#include "los_mip_udp.h"
#include "los_mip_osfunc.h"

static u16_t ipv4_id = 0;

int los_mip_make_ipaddr(ip_addr_t *addr, u32_t seg1, u32_t seg2, u32_t seg3,u32_t seg4)
{
    if (NULL == addr)
    {
        return -MIP_ERR_PARAM;
    }
    if (sizeof(struct ip4_addr) != sizeof(*addr))
    {
        return -MIP_ERR_PARAM;
    }
    addr->addr = 0;
    addr->addr |= (seg4&0xff) << 24;
    addr->addr |= (seg3&0xff) << 16;
    addr->addr |= (seg2&0xff) << 8;
    addr->addr |= seg1&0xff ;
    
    return MIP_OK;
}

u32_t los_mip_checksum(unsigned char *buf, int len)
{
    u32_t sum = 0;
    if (NULL == buf || 0 == len)
    {
        return 0;
    }
    /* build the sum of 16bit words */
    while(len > 1 )
    {
        sum += 0xFFFF & (*buf<<8|*(buf+1));
        buf+=2;
        len-=2;
    }
    /* if there is a byte left then add it (padded with zero) */ 
    if (len)
    {
        sum += (0xFF & *buf)<<8;
    }
    /* 
       now calculate the sum over the bytes in the sum
       until the result is only 16bit long 
    */
    while (sum>>16)
    {
        sum = (sum & 0xFFFF)+(sum >> 16);
    }
    /* build 1's complement: */
    return( (u32_t) sum ^ 0xFFFF);
}

u32_t los_mip_checksum_pseudo_ipv4(struct netbuf *p, u8_t proto, u16_t tlen, ip_addr_t *src, ip_addr_t *dest)
{
    
    u32_t sum = 0;
    u32_t addr = 0;
    int len = 0;
    u8_t *buf = NULL;
    
    if (NULL == p || 0 == tlen)
    {
        return 0x0;
    }
    
    addr = src->addr;
    sum += MIP_HTONS(addr & 0xffffUL);
    sum += MIP_HTONS((addr>>16) & 0xffffUL);
    addr = dest->addr;
    sum += MIP_HTONS(addr & 0xffffUL);
    sum += MIP_HTONS((addr>>16) & 0xffffUL);
    

    sum += ((MIP_HTONS(0x00<<8)| proto) & 0xffffUL);
    sum += (tlen & 0xffffUL);      
    /* build the sum of 16bit words */
    buf = p->payload;
    
    len = p->len;
    while(len > 1 )
    {
        sum += 0xFFFF & (*buf<<8|*(buf+1));
        buf+=2;
        len-=2;
    }
    /* if there is a byte left then add it (padded with zero) */
    if (len)
    {
        sum += (0xFF & *buf)<<8;
    }
    /*
        now calculate the sum over the bytes in the sum
        until the result is only 16bit long
    */
    while (sum>>16)
    {
        sum = (sum & 0xFFFF)+(sum >> 16);
    }
    /* build 1's complement: */
    return( (u32_t)sum ^ 0xFFFF );
}


int los_mip_is_multicast(struct ip4_addr *address)
{
    if(NULL == address)
    {
        return MIP_FALSE;
    }
    
    if((address->addr & MIP_HTONL(0xf0000000UL)) == MIP_HTONL(0xe0000000UL))
    {
        return MIP_TRUE;
    }
    return MIP_FALSE;
}

int los_mip_ipv4_input(struct netbuf *p, struct netif *dev)
{
    int ret = 0;
    u16_t iph_len = 0;
    u16_t ip_totallen = 0;
    u16_t ip_checksum = 0;
    struct ip4_addr dest;
    struct ip4_addr src;
    int delfalg = 0;
    struct ipv4_hdr *iph = NULL;
    
    if(NULL == p || NULL == dev)
    {
        return -MIP_ERR_PARAM;
    }
    iph = (struct ipv4_hdr *)p->payload;
    
    /* check header ver/len/checksum .... */
    if(MIP_IP_V4 != MIP_IPH(iph->vhl))
    {
        netbuf_free(p);
        return -MIP_IP_HEAD_VER_ERR;
    }
    iph_len = 4 * MIP_IPHLEN(iph->vhl);
    ip_totallen = MIP_NTOHS(iph->len);
    
    if(iph_len > p->len || ip_totallen > p->len)
    {
        netbuf_free(p);
        return -MIP_IP_LEN_ERR;
    }
    
    if(iph_len > MIP_IP_MIN_HLEN)
    {
        /* have otpions maybe igmp packages , not supported yet */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    if(p->len != ip_totallen)
    {
        /* some padding data at the last , all zero data, so we relength the data length */
        p->len = ip_totallen;
    }
    
    ip_checksum = (u16_t)los_mip_checksum(p->payload, iph_len);
    if(ip_checksum != 0)
    {
        netbuf_free(p);
        return -MIP_IP_HDR_CHECKSUM_ERR;
    }
    
    memcpy(&dest, &iph->dest, sizeof(struct ip4_addr));
    memcpy(&src, &iph->src, sizeof(struct ip4_addr));
    /* start to find if this ip package is for us */
    if(MIP_TRUE == los_mip_is_multicast(&dest))
    {
        /* we do not support multicast yet */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    
    if((dev->flags & NETIF_FLAG_UP)
        && dev->ip_addr.addr == MIP_HTONL(MIP_IP_ANY)
        && iph->proto == MIP_PRT_UDP
        /* && dest.addr != MIP_HTONL(MIP_IP_BROADCAST) */
        && dev->autoipflag != AUTO_IP_PROCESSING)
    {
        /* 
            ethnet not configed and maybe dhcp messages, 
            if dhcp not start, dscard msg, else need pass data to up layer 
        */
        netbuf_free(p);
        return -MIP_IP_NOT_FOR_US;
    }
    if(dev->ip_addr.addr != MIP_HTONL(MIP_IP_ANY)
        && dest.addr != MIP_HTONL(MIP_IP_BROADCAST)
        && dev->ip_addr.addr != dest.addr)
    {
        /* ethernet configed and ip not for us */
        netbuf_free(p);
        return -MIP_IP_NOT_FOR_US;
    }
    
    if((iph->offset & MIP_HTONS(MIP_IP_OFFMASK | MIP_IP_MF)) != 0) 
    {
        /* ip fragment messages, we do not support yet */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    los_mip_jump_header(p, iph_len);
    switch(iph->proto)
    {
        case MIP_PRT_UDP:
            ret = los_mip_udp_input(p, dev, &iph->src, &iph->dest); 
            break;
        case MIP_PRT_TCP:
            delfalg = 1;
            break;
        case MIP_PRT_ICMP:
            ret = los_mip_icmp_input(p, dev, &iph->src, &iph->dest);
            break;
        default:
            delfalg = 1;
            break;
    }
    if(delfalg)
    {
        /* pacage not be processed , so free it */
        netbuf_free(p);
    }
    
    return ret;
}

int los_mip_ipv4_output(struct netif *dev, struct netbuf *p, ip_addr_t *src, ip_addr_t *dst,
             u8_t ttl, u8_t tos,
             u8_t proto)
{
    u16_t iph_len = 0;
    u16_t ip_checksum = 0;
    struct ipv4_hdr *iph = NULL;
    
    if(NULL == p || NULL == dev)
    {
        return -MIP_ERR_PARAM;
    }
    iph_len = sizeof(struct ipv4_hdr);
    los_mip_jump_header(p, -iph_len);
    
    iph = (struct ipv4_hdr *)p->payload;
    memset(iph, 0, iph_len);
    iph->vhl= MIP_IPV4_NOOPT_VL;
    iph->tos = tos;
    iph->len = MIP_HTONS(p->len);
    
    if (ipv4_id != 0)
    {
        ipv4_id++;
    }
    else
    {
        ipv4_id = (u16_t)los_mip_cur_tm();;
    }
    iph->id = MIP_HTONS(ipv4_id);
    iph->ttl = ttl;
    iph->proto = proto;
    if(NULL != src)
    {
        iph->src = *src;
    }
    if(NULL != dst)
    {
        iph->dest = *dst;
    }
    ip_checksum = (u16_t)los_mip_checksum(p->payload, iph_len);
    iph->chksum = MIP_HTONS(ip_checksum);
    
    if(dev->mtu && dev->mtu < p->len)
    {
        /*
            not support yet, maybe later
            fragment the ip package
        */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }

    return dev->arp_xmit(dev, p, dst);
}

struct netif *los_mip_ip_route(ip_addr_t *dst)
{
    struct netif *dev = NULL;
    if (NULL == dst)
    {
        return NULL;
    }
    dev = los_mip_get_netif_list();
    if(NULL == dev)
    {
        return NULL;
    }
    while(NULL != dev)
    {
        if ((dst->addr & dev->netmask.addr) == (dev->ip_addr.addr & dev->netmask.addr))
        {
            break;
        }
        dev = dev->next;
    }
    if(NULL == dev)
    {
        dev = los_mip_get_default_if();
    }
    return dev;
}
