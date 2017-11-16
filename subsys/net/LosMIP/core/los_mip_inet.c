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
#include "los_mip_udp.h"
#include "los_mip_inet.h"

/*ip version 4 string address buf length */
#define MIP_MAX_IP4_ASCII_LEN 16
/*ip version 4 string address buf */
static char g_ip_ascii[MIP_MAX_IP4_ASCII_LEN] = {0};

/*****************************************************************************
 Function    : inet_addr
 Description : this function is just only for ipv4 ascii internet address 
               interpretation.returned is in network order.
 Input       : ipaddr @ string format ip ver 4 address
 Output      : None
 Return      : network order ip address
 *****************************************************************************/
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
        if ((a > 255) || (b > 255) || (c > 255) || (d > 255)
            || (a < 0) || (b < 0) || (c < 0) || (d < 0))
        {
            return MIP_IP_ANY;
        }
        ipa = ((d&0x000000ff)<<24) | ((c&0x000000ff)<<16) | ((b&0x000000ff)<<8) | ((a&0x000000ff));
        return ipa;
    }
    
    return MIP_IP_ANY;
}

/*****************************************************************************
 Function    : inet_aton
 Description : change string ip address to network byte order ip address
 Input       : outip @ string format ip ver 4 address
 Output      : addr @ network byte order ip ver 4 address
 Return      : MIP_OK address process ok.
 *****************************************************************************/
int inet_aton(const char *outip, struct in_addr* addr)
{
    u32_t data = 0;
    
    if ((NULL == outip) || (NULL == addr))
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

/*****************************************************************************
 Function    : inet_aton
 Description : change network byte order ip address to string ip address 
 Input       : addr @ network byte order ip address
 Output      : None
 Return      : g_ip_ascii @ string ip address's pointer
 *****************************************************************************/
char *inet_ntoa(const ip_addr_t *addr)
{
    memset(g_ip_ascii, 0, MIP_MAX_IP4_ASCII_LEN);
    sprintf(g_ip_ascii, "%d.%d.%d.%d", addr->addr&0xff, 
            (addr->addr&0xff00)>>8, 
            (addr->addr&0xff0000)>>16, 
            (addr->addr&0xff000000)>>24);
    return g_ip_ascii;
}

/*****************************************************************************
 Function    : htons
 Description : host to network byte order for unsigned short data
 Input       : n @ host order data
 Output      : 
 Return      : u16_t network order data
 *****************************************************************************/
u16_t htons(u16_t n)
{
    return MIP_HTONS(n);
}

/*****************************************************************************
 Function    : ntohs
 Description : network byte order to host order  for unsigned short data 
 Input       : n @ network order data
 Output      : 
 Return      : u16_t host order data
 *****************************************************************************/
u16_t ntohs(u16_t n)
{
    return htons(n);
}

/*****************************************************************************
 Function    : htonl
 Description : host to network byte order for unsigned long data 
 Input       : n @ host order data
 Output      : 
 Return      : u32_t network order data
 *****************************************************************************/
u32_t htonl(u32_t n)
{
    return MIP_HTONL(n);
}

/*****************************************************************************
 Function    : ntohl
 Description : network byte order to host order  for unsigned long data 
 Input       : n @ network order data
 Output      : 
 Return      : u32_t host order data
 *****************************************************************************/
u32_t ntohl(u32_t n)
{
    return htonl(n);
}
