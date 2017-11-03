#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"
#include "los_mip_ethernet.h"
#include "los_mip_ipv4.h"
#include "los_mip_udp.h"

#include "los_mip_inet.h"

#define MIP_MAX_IP4_ASCII_LEN 16
static char g_ip_ascii[MIP_MAX_IP4_ASCII_LEN] = {0};

/**
 * this function is just only for ipv4
 * ascii internet address interpretation.
 * returned is in network order.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @return ip address in network order
 */
u32_t inet_addr(const char *ipaddr)
{
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int ret = 0;
    u32_t ipa = 0;
    if (NULL == ipaddr)
    {
        return MIP_IP_ANY;
    }
    ret = sscanf(ipaddr, "%d.%d.%d.%d", &a,&b,&c,&d);
    if (ret == 4)
    {
        if (a > 255 || b > 255 || c > 255 || d > 255
            || a < 0 || b < 0 || c < 0 || d < 0)
        {
            return MIP_IP_ANY;
        }
        ipa = ((d&0x000000ff)<<24) | ((c&0x000000ff)<<16) | ((b&0x000000ff)<<8) | ((a&0x000000ff));
        return ipa;
    }
    
    return MIP_IP_ANY;
}

int inet_aton(const char *outip, struct in_addr* addr)
{
    u32_t data = 0;
    if (NULL == outip || NULL == addr)
    {
        return -MIP_ERR_PARAM;
    }
    
    data = inet_addr(outip);
    if (MIP_IP_ANY == data)
    {
        return -MIP_ERR_PARAM;
    }
    
    addr->S_un.S_addr = data;
    return MIP_OK;
}

char *inet_ntoa(const ip_addr_t *addr)
{
    memset(g_ip_ascii, 0, MIP_MAX_IP4_ASCII_LEN);
    sprintf(g_ip_ascii, "%d.%d.%d.%d", addr->addr&0xff, 
            (addr->addr&0xff00)>>8, 
            (addr->addr&0xff0000)>>16, 
            (addr->addr&0xff000000)>>24);
    return g_ip_ascii;
}

u16_t htons(u16_t n)
{
    return MIP_HTONS(n);
}

u16_t ntohs(u16_t n)
{
    return htons(n);
}

u32_t htonl(u32_t n)
{
    return MIP_HTONL(n);
}

u32_t ntohl(u32_t n)
{
    return htonl(n);
}
