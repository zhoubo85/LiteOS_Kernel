#ifndef _LOS_MIP_IPV4_H
#define _LOS_MIP_IPV4_H

#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipaddr.h"

#define MIP_IP_RF 0x8000U        /* reserved fragment flag */
#define MIP_IP_DF 0x4000U        /* don't fragment flag */
#define MIP_IP_MF 0x2000U        /* more fragments flag */
#define MIP_IP_OFFMASK 0x1fffU   /* mask for fragmenting bits */

#define MIP_IP_V4 0x4
#define MIP_IPH(x) (((x)&0xf0)>>4)
#define MIP_IPHLEN(x) ((x)&0x0f)

#define MIP_PRT_ICMP    1
#define MIP_PRT_UDP     17
#define MIP_PRT_TCP     6

#define MIP_IP_MIN_HLEN 20

#define MIP_IPV4_NOOPT_VL 0x45

/* The IP header for ipv4 */
PACK_STRUCT_BEGIN
struct ipv4_hdr 
{
    PACK_STRUCT_FIELD(u8_t vhl);        /* version and header length */
    PACK_STRUCT_FIELD(u8_t tos);        /* type of service */
    PACK_STRUCT_FIELD(u16_t len);       /* total length */
    PACK_STRUCT_FIELD(u16_t id);        /* identification */ 
    PACK_STRUCT_FIELD(u16_t offset);    /* fragment offset field */
    PACK_STRUCT_FIELD(u8_t ttl);        /* time to live */ 
    PACK_STRUCT_FIELD(u8_t proto);      /* protocol */
    PACK_STRUCT_FIELD(u16_t chksum);    /* checksum */
    PACK_STRUCT_FIELD(struct ip4_addr src);     /* source IP addresses */
    PACK_STRUCT_FIELD(struct ip4_addr dest);    /* destination IP addresses */ 
};
PACK_STRUCT_END

int los_mip_ipv4_input(struct netbuf *p, struct netif *dev);
int los_mip_make_ipaddr(ip_addr_t *addr, u32_t seg1, u32_t seg2, u32_t seg3,u32_t seg4);
int los_mip_ipv4_output(struct netif *dev, struct netbuf *p, ip_addr_t *src, ip_addr_t *dest,
             u8_t ttl, u8_t tos,
             u8_t proto);
int los_mip_is_multicast(struct ip4_addr *address);
struct netif *los_mip_ip_route(ip_addr_t *dst);
u32_t los_mip_checksum(unsigned char *buf, int len);
u32_t los_mip_checksum_pseudo_ipv4(struct netbuf *p, 
                                u8_t proto, 
                                u16_t tlen, 
                                ip_addr_t *src, 
                                ip_addr_t *dest);
#endif
