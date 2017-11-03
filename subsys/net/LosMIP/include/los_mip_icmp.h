#ifndef _LOS_MIP_ICMP_H
#define _LOS_MIP_ICMP_H

#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipv4.h"
#include "los_mip_ipaddr.h"
#include "los_mip_netbuf.h"
#include "los_mip_connect.h"

#define ICMP_TYPE_ECHO 0x08
#define ICMP_TYPE_ECHO_REPLY 0x00
#define ICMP_HEADER_LEN 8


PACK_STRUCT_BEGIN
struct icmp_hdr {
  PACK_STRUCT_FIELD(u8_t type);
  PACK_STRUCT_FIELD(u8_t code);  // src/dest UDP ports 
  PACK_STRUCT_FIELD(u16_t chksum);
  PACK_STRUCT_FIELD(u16_t id);
  PACK_STRUCT_FIELD(u16_t sequence);
};
PACK_STRUCT_END


int los_mip_icmp_input(struct netbuf *p, struct netif *dev, ip_addr_t *src, ip_addr_t *dst);
int los_mip_icmp_echo_reply(struct netbuf *p, struct netif *dev, ip_addr_t *dst);

#endif
