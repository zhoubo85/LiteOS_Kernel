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

/* ip package header id */
static u16_t ipv4_id = 0;

/*****************************************************************************
 Function    : los_mip_make_ipaddr
 Description : create a ip v4 address from given number. 
               for example 192.168.2.45, seg1 is 192 seg2 is 168 ...
 Input       : seg1 @ fist address data 
               seg2 @ second address data
               seg3 @ third address data
               seg4 @ fourth address data
 Output      : addr @ network byte order ip ver4 address.
 Return      : MIP_OK process ok.
 *****************************************************************************/
int los_mip_make_ipaddr(ip_addr_t *addr, 
                        u32_t seg1, 
                        u32_t seg2, 
                        u32_t seg3,
                        u32_t seg4)
{
    if (NULL == addr)
    {
        return -MIP_ERR_PARAM;
    }
    if (sizeof(struct ipv4_addr) != sizeof(*addr))
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

/*****************************************************************************
 Function    : los_mip_checksum
 Description : calc the checksum of the ip package.
 Input       : buf @ ip package's buf pointer
               len @ ip header's length
 Output      : None
 Return      : sum @ the result of the checksum.
 *****************************************************************************/
u32_t los_mip_checksum(unsigned char *buf, int len)
{
    u32_t sum = 0;
    
    if ((NULL == buf) || (0 == len))
    {
        return 0;
    }
    /* build the sum of 16bit words */
    while (len > 1 )
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

/*****************************************************************************
 Function    : los_mip_checksum_pseudo_ipv4
 Description : calc the checksum of the pseudo udp or tcp package.
 Input       : p @ ip package's buf pointer
               tlen @ total length of tcp or udp
               proto @ protocol type 
               src @ source ip address
               dst @ dest ip address
 Output      : None
 Return      : sum @ the result of the checksum.
 *****************************************************************************/
u32_t los_mip_checksum_pseudo_ipv4(struct netbuf *p, 
                                   u8_t proto, 
                                   u16_t tlen, 
                                   ip_addr_t *src, 
                                   ip_addr_t *dest)
{
    
    u32_t sum = 0;
    u32_t addr = 0;
    int len = 0;
    u8_t *buf = NULL;
    
    if ((NULL == p) || (0 == tlen))
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
    while (len > 1 )
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

/*****************************************************************************
 Function    : los_mip_is_multicast
 Description : judge if the ip ver4 address is multicast ip address
 Input       : address @ the ip ver4 address pointer
 Output      : None
 Return      : MIP_TRUE ip is multicast ip, MIP_FALSE not multicast ip
 *****************************************************************************/
int los_mip_is_multicast(struct ipv4_addr *address)
{
    if (NULL == address)
    {
        return MIP_FALSE;
    }
    
    if ((address->addr & MIP_HTONL(0xf0000000UL)) == MIP_HTONL(0xe0000000UL))
    {
        return MIP_TRUE;
    }
    return MIP_FALSE;
}

/*****************************************************************************
 Function    : los_mip_ipv4_input
 Description : ip package process function.parse the ip header and call upper 
               layer functions to do upper layer process.
 Input       : p @ ip package buf pointer
               dev @ net dev's handle which the data come from
 Output      : None
 Return      : 0(MIP_OK) process ok, other value means some error happended.
 *****************************************************************************/
int los_mip_ipv4_input(struct netbuf *p, struct netif *dev)
{
    int ret = 0;
    u16_t iph_len = 0;
    u16_t ip_totallen = 0;
    u16_t ip_checksum = 0;
    struct ipv4_addr dest;
    struct ipv4_addr src;
    int delfalg = 0;
    struct ipv4_hdr *iph = NULL;
    
    if((NULL == p) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    iph = (struct ipv4_hdr *)p->payload;
    
    /* check header ver/len/checksum .... */
    if (MIP_IP_V4 != MIP_IPH(iph->vhl))
    {
        netbuf_free(p);
        return -MIP_IP_HEAD_VER_ERR;
    }
    iph_len = 4 * MIP_IPHLEN(iph->vhl);
    ip_totallen = MIP_NTOHS(iph->len);
    
    if ((iph_len > p->len) || (ip_totallen > p->len))
    {
        netbuf_free(p);
        return -MIP_IP_LEN_ERR;
    }
    
    if (iph_len > MIP_IP_MIN_HLEN)
    {
        /* have otpions maybe igmp packages , not supported yet */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    if (p->len != ip_totallen)
    {
        /* some padding data at the last , all zero data, so we relength the data length */
        p->len = ip_totallen;
    }
    
    ip_checksum = (u16_t)los_mip_checksum(p->payload, iph_len);
    if (ip_checksum != 0)
    {
        netbuf_free(p);
        return -MIP_IP_HDR_CHECKSUM_ERR;
    }
    
    memcpy(&dest, &iph->dest, sizeof(struct ipv4_addr));
    memcpy(&src, &iph->src, sizeof(struct ipv4_addr));
    /* start to find if this ip package is for us */
    if (MIP_TRUE == los_mip_is_multicast(&dest))
    {
        /* we do not support multicast yet */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    
    if ((dev->flags & NETIF_FLAG_UP)
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
    if (dev->ip_addr.addr != MIP_HTONL(MIP_IP_ANY)
        && dest.addr != MIP_HTONL(MIP_IP_BROADCAST)
        && dev->ip_addr.addr != dest.addr)
    {
        /* ethernet configed and ip not for us */
        netbuf_free(p);
        return -MIP_IP_NOT_FOR_US;
    }
    
    if ((iph->offset & MIP_HTONS(MIP_IP_OFFMASK | MIP_IP_MF)) != 0) 
    {
        /* ip fragment messages, we do not support yet */
        netbuf_free(p);
        return -MIP_IP_NOT_SUPPORTED;
    }
    los_mip_jump_header(p, iph_len);
    switch (iph->proto)
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
    if (delfalg)
    {
        /* pacage not be processed , so free it */
        netbuf_free(p);
    }
    
    return ret;
}

/*****************************************************************************
 Function    : los_mip_ipv4_output
 Description : transmit the ip package out.
 Input       : dev @ net dev's handle which the data send to
               p @ ip package buf pointer
               src @ source ip address
               dst @ dest ip address
               ttl @ Time To Live
               tos @ type of service
               proto @ upper layer protocol
 Output      : None
 Return      : 0(MIP_OK) send out ok, other value means error happended.
 *****************************************************************************/
int los_mip_ipv4_output(struct netif *dev, 
                        struct netbuf *p, ip_addr_t *src, 
                        ip_addr_t *dst,
                        u8_t ttl, u8_t tos,
                        u8_t proto)
{
    u16_t iph_len = 0;
    u16_t ip_checksum = 0;
    struct ipv4_hdr *iph = NULL;
    
    if((NULL == p) || (NULL == dev))
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
        ipv4_id = (u16_t)los_mip_cur_tm();
    }
    iph->id = MIP_HTONS(ipv4_id);
    iph->ttl = ttl;
    iph->proto = proto;
    if (NULL != src)
    {
        iph->src = *src;
    }
    if (NULL != dst)
    {
        iph->dest = *dst;
    }
    ip_checksum = (u16_t)los_mip_checksum(p->payload, iph_len);
    iph->chksum = MIP_HTONS(ip_checksum);
    
    if (dev->mtu && (dev->mtu < p->len))
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

/*****************************************************************************
 Function    : los_mip_ip_route
 Description : find which dev to process the package
 Input       : dst @ dest ip address of the input package.
 Output      : None
 Return      : dev @ device's pointer which to process the package
 *****************************************************************************/
struct netif *los_mip_ip_route(ip_addr_t *dst)
{
    struct netif *dev = NULL;
    if (NULL == dst)
    {
        return NULL;
    }
    dev = los_mip_get_netif_list();
    if (NULL == dev)
    {
        return NULL;
    }
    while (NULL != dev)
    {
        if ((dst->addr & dev->netmask.addr) == (dev->ip_addr.addr & dev->netmask.addr))
        {
            break;
        }
        dev = dev->next;
    }
    if (NULL == dev)
    {
        dev = los_mip_get_default_if();
    }
    return dev;
}
