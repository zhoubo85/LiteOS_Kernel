#ifndef _LOS_MIP_IPADDR_H
#define _LOS_MIP_IPADDR_H

#include "los_mip_typed.h"

#define MIP_ETH_MAC_LEN 6
#define MIP_IP_ANY ((u32_t)0x00000000)
#define MIP_IP_BROADCAST    ((u32_t)0xffffffffUL)

#define MIP_IP_DEFAULT_TTL 0x7F

PACK_STRUCT_BEGIN
struct ip4_addr 
{
    PACK_STRUCT_FIELD(u32_t addr;)
};
PACK_STRUCT_END

PACK_STRUCT_BEGIN
struct eth_mac 
{
    PACK_STRUCT_FIELD(u8_t hwaddr[MIP_ETH_MAC_LEN];)
};
PACK_STRUCT_END

typedef struct ip4_addr ip_addr_t;


#define IP_V4_ADDR_SIZE 4
#define IP_V6_ADDR_SIZE 16


#endif
