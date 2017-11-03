#ifndef _LOS_MIP_INET_H
#define _LOS_MIP_INET_H

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

#define INADDR_ANY MIP_IP_ANY

struct in_addr
{
    union
    {
        struct
        {
            u8_t s_b1,s_b2,s_b3,s_b4;
        } S_un_b; /* An IPv4 address formatted as four u_chars. */
        struct
        {
            u16_t s_w1,s_w2;
        } S_un_w; /* An IPv4 address formatted as two u_shorts */
        u32_t S_addr;/* An IPv4 address formatted as a u_long */
    } S_un;
#define s_addr S_un.S_addr
};

struct sockaddr_in 
{
    u8_t sin_len;
    u8_t sin_family;
    u16_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr 
{
    u8_t sa_len;
    u8_t sa_family;
    char sa_data[14];
};

struct timeval 
{
    long    tv_sec;         /* seconds */
    long    tv_usec;        /* and microseconds */
};


u16_t htons(u16_t n);
u16_t ntohs(u16_t n);
u32_t htonl(u32_t n);
u32_t ntohl(u32_t n);

u32_t inet_addr(const char *ipaddr);

#endif
