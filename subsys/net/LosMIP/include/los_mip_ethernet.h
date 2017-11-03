#ifndef _LOS_MIP_ETHERNET_H
#define _LOS_MIP_ETHERNET_H

#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"

#define MIP_ETH_HW_LEN 6
#define MIP_ETH_HDR_LEN 14

enum mip_eth_types 
{
    /** ipv4 type */
    TYPE_IPV4        = 0x0800U,
    /** arp type */
    TYPE_ARP       = 0x0806U, 
    /** ipv6 type */
    TYPE_IPV6      = 0x86DDU,
    TYPE_MAX
};

PACK_STRUCT_BEGIN
struct eth_hdr
{
    PACK_STRUCT_FIELD(struct eth_mac dst;)
    PACK_STRUCT_FIELD(struct eth_mac src;)
    PACK_STRUCT_FIELD(u16_t type;)
};
PACK_STRUCT_END

int los_mip_eth_arp_output(struct netif *dev, 
                           struct netbuf *p,
                           struct eth_mac *src, 
                           struct eth_mac *dst);
int los_mip_eth_ip_output(struct netif *dev, 
                          struct netbuf *p, 
                          struct eth_mac *src, 
                          struct eth_mac *dst,
                          enum mip_eth_types type);
int los_mip_eth_input(struct netbuf *p, struct netif *dev);

u16_t los_mip_get_ip_ver(struct netbuf *p);
int los_mip_set_ip_ver(struct netbuf *p, u16_t ver);



#endif
