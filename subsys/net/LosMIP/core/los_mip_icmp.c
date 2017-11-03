#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_ipv4.h"
#include "los_mip_icmp.h"


int los_mip_icmp_input(struct netbuf *p, struct netif *dev, ip_addr_t *src, ip_addr_t *dst)
{
    struct icmp_hdr *icmph = NULL;
    ip_addr_t srctmp;
    
    if (NULL == p || NULL == dev)
    {
        return -MIP_ERR_PARAM;
    }
    if (p->len < ICMP_HEADER_LEN)
    {
        netbuf_free(p);
        return -MIP_ICMP_PKG_LEN_ERR;
    }
    icmph = (struct icmp_hdr *)p->payload;
    
    switch(icmph->type)
    {
        case ICMP_TYPE_ECHO:
            icmph->chksum = los_mip_checksum(p->payload, p->len);
            if (icmph->chksum != 0)
            {
                break;
            }
            icmph->type = ICMP_TYPE_ECHO_REPLY;
            srctmp.addr = src->addr;
            los_mip_icmp_echo_reply(p, dev, &srctmp);
            break;
        default:
            break;
    }
    netbuf_free(p);
    return MIP_OK;
}

int los_mip_icmp_echo_reply(struct netbuf *p, struct netif *dev, ip_addr_t *dst)
{
    int ret = 0;
    
    ret = los_mip_ipv4_output(dev, p, &dev->ip_addr, dst,
        0x40,  0, MIP_PRT_ICMP);
    return ret;
}
