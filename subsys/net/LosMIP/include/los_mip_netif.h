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
 
#ifndef _LOS_MIP_NETIF_H
#define _LOS_MIP_NETIF_H

#include "los_mip_typed.h"
#include "los_mip_ipaddr.h"
#include "los_mip_netbuf.h"

struct netif;
#define MIP_ETH_HWADDR_LEN    6
#define NETIF_MAX_HWADDR_LEN 6U
typedef int (*netif_input_func)(struct netbuf *p, struct netif *inp);
typedef int (*netif_arpxmit_func)(struct netif *netif, struct netbuf *p, ip_addr_t *ipaddr);
typedef int (*netif_hwxmit_func)(struct netif *netif, struct netbuf *p);

struct netif 
{
    /* pointer to next in linked list */
    struct netif *next;

    /* IP address configuration in network byte order */
    ip_addr_t ip_addr;
    ip_addr_t netmask;
    ip_addr_t gw;

    /* 
        This function is called by the network device driver
        to pass a packet up the TCP/IP stack. 
    */
    netif_input_func input;

    /*
        This function is called by the IP module when it wants
        to send a packet on the interface. This function typically
        first resolves the hardware address, then sends the packet.
    */
    netif_arpxmit_func arp_xmit;
    netif_hwxmit_func hw_xmit;

    /*
        This field can be set by the device driver and could point
        to state information for the device. 
    */
    void *state;
    u8_t autoipflag;
    /* maximum transfer unit (in bytes) */
    u16_t mtu;
    /* number of bytes used in hwaddr */
    u8_t hwaddr_len;
    /* link level hardware address of this interface */
    u8_t hwaddr[NETIF_MAX_HWADDR_LEN];
    u8_t flags;
    /* descriptive abbreviation */
    char name[2];
    /* number of this interface */
    u8_t num;

};

#define AUTO_IP_PROCESSING 1
#define AUTO_IP_DONE 2

/*
    Whether the network interface is 'up'. This is
    a software flag used to control whether this network
    interface is enabled and processes traffic.
    It must be set by the startup code before this netif can be used
*/
#define NETIF_FLAG_UP           0x01U
/*
    If set, the netif has broadcast capability.
    Set by the netif driver in its init function. 
*/
#define NETIF_FLAG_BROADCAST    0x02U
/*
    If set, the interface has an active link
    (set by the network interface driver).
    Either set by the netif driver in its init function (if the link
    is up at that time) or at a later point once the link comes up
    (if link detection is supported by the hardware). 
*/
#define NETIF_FLAG_LINK_UP      0x04U
/*
    If set, the netif is an ethernet device using ARP.
    Set by the netif driver in its init function.
    Used to check input packet types and use of DHCP. 
*/
#define NETIF_FLAG_ETHARP       0x08U
/*
    If set, the netif is an ethernet device. It might not use
    ARP or TCP/IP if it is used for PPPoE only.
*/
#define NETIF_FLAG_ETHERNET     0x10U


int los_mip_netif_add(struct netif *dev);
int los_mip_netif_remove(struct netif *dev);
int los_mip_netif_config( struct netif *dev, 
                        ip_addr_t *ipaddr, 
                        ip_addr_t *netmask,
                        ip_addr_t *gw );
int los_mip_netif_up(struct netif *dev);
int los_mip_netif_down(struct netif *dev);
struct netif *los_mip_get_netif_list(void);
void los_mip_set_default_if(struct netif *dev);
struct netif * los_mip_get_default_if(void);

#endif
