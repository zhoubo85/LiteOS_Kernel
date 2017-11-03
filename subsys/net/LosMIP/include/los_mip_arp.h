#ifndef _LOS_MIP_ARP_H
#define _LOS_MIP_ARP_H

#include <stdlib.h>
#include <string.h>

#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"
#include "los_mip_ipaddr.h"


#define MIP_ARP_HEADER_LEN 28

/* ARP hwtype values */
enum mip_arp_hwtype 
{
    TYPE_ETH = 1//only support ethernet now
};

/* ARP message types (opcodes) */
enum mip_arp_opcode 
{
    ARP_REQ = 1,
    ARP_REP = 2,
    ARP_MAX
};


PACK_STRUCT_BEGIN
struct mip_arp_hdr 
{
    PACK_STRUCT_FIELD(u16_t hwtype;)
    PACK_STRUCT_FIELD(u16_t proto;)
    PACK_STRUCT_FIELD(u8_t  hwlen;)
    PACK_STRUCT_FIELD(u8_t  protolen;)
    PACK_STRUCT_FIELD(u16_t opcode;)
    PACK_STRUCT_FIELD(struct eth_mac sndmac;)
    PACK_STRUCT_FIELD(struct ip4_addr sndip;)
    PACK_STRUCT_FIELD(struct eth_mac dstmac;)
    PACK_STRUCT_FIELD(struct ip4_addr dstip;)
};
PACK_STRUCT_END

struct arp_table
{
    struct eth_mac hw;
    struct ip4_addr ip;
    u32_t tm;
};

int los_mip_arp_request(struct netif *dev, ip_addr_t *ipaddr);
int los_mip_arp_xmit(struct netif *dev, struct netbuf *p, ip_addr_t *dstip);
int los_mip_arp_input(struct netif *dev, struct netbuf *p);
int los_mip_arp_tab_init(void);
int los_mip_find_arp_entry(struct ip4_addr *ip, struct eth_mac **dstmac);

#endif
