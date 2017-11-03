#ifndef _LOS_MIP_UDP_H
#define _LOS_MIP_UDP_H

#include "los_mip_typed.h"
#include "los_mip_netif.h"
#include "los_mip_ipv4.h"
#include "los_mip_ipaddr.h"
#include "los_mip_netbuf.h"
#include "los_mip_connect.h"


PACK_STRUCT_BEGIN
struct udp_hdr {
  PACK_STRUCT_FIELD(u16_t src);
  PACK_STRUCT_FIELD(u16_t dest);  /* src/dest UDP ports */
  PACK_STRUCT_FIELD(u16_t len);
  PACK_STRUCT_FIELD(u16_t chksum);
};
PACK_STRUCT_END

#define MIP_UDP_HDR_LEN 8

#define MIP_DHCP_SER_PORT 0x0043
#define MIP_DHCP_CLI_PORT 0x0044

struct udp_ctl;
/* @addr the remote IP address from which the packet was received */
typedef void (*udp_recv_func)(void *arg, struct udp_ctl *pcb, struct netbuf *p, const ip_addr_t *addr, u16_t port);


struct udp_ctl
{
    struct udp_ctl *next;
    ip_addr_t localip;
    u16_t lport;            /* local port */
    ip_addr_t remote_ip; 
    u16_t rport;            /*remote port */
    u8_t so_opt;            /* Socket options */ 
    u8_t tos;               /* Type Of Service */           
    u8_t ttl;               /* Time To Live */
    udp_recv_func rcvfn;
    void *rcvarg;
};


int los_mip_udp_input(struct netbuf *p, struct netif *dev, ip_addr_t *src, ip_addr_t *dst);

int los_mip_udp_output( struct udp_ctl *udpctl, struct netbuf *p, ip_addr_t *dst_ip, u16_t dst_port);
int los_mip_udp_bind(struct udp_ctl *udpctl, u32_t *dst_ip, u16_t dst_port);

#endif
