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
#include "los_mip_tcpip_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* network device list */
struct netif *g_netif_list = NULL;
/* default network device */
struct netif *g_if_default = NULL;

/*****************************************************************************
 Function    : los_mip_get_netif_list
 Description : get netif list header pointer.
 Input       : None
 Output      : None
 Return      : g_netif_list @ netif list header pointer.
 *****************************************************************************/
struct netif *los_mip_get_netif_list(void)
{
    return g_netif_list;
}

/*****************************************************************************
 Function    : los_mip_set_default_if
 Description : set default network device
 Input       : dev @ device's pointer that set as default network device
 Output      : None
 Return      : None
 *****************************************************************************/
void los_mip_set_default_if(struct netif *dev)
{
    g_if_default = dev;
}

/*****************************************************************************
 Function    : los_mip_get_default_if
 Description : get default network device
 Input       : None
 Output      : None
 Return      : g_if_default @ default network device's pointer
 *****************************************************************************/
struct netif * los_mip_get_default_if(void)
{
    return g_if_default;
}

/*****************************************************************************
 Function    : los_mip_netif_add
 Description : add network device to dev list
 Input       : dev @ device's pointer that need add to dev list
 Output      : None
 Return      : MIP_OK add ok.
 *****************************************************************************/
int los_mip_netif_add(struct netif *dev)
{
    if (NULL == dev)
    {
        return MIP_OK;
    }
    if (NULL == g_netif_list)
    {
        g_netif_list = dev;
        dev->next = NULL;
    }
    else
    {
        dev->next = g_netif_list;
        g_netif_list = dev;
    }
    dev->input = los_mip_tcpip_inpkt;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_netif_remove
 Description : remove network device from dev list
 Input       : dev @ device's pointer that need remove from dev list
 Output      : None
 Return      : MIP_OK remove ok
 *****************************************************************************/
int los_mip_netif_remove(struct netif *dev)
{
    struct netif *cur = NULL;
    struct netif *before = NULL;
    if (NULL == dev)
    {
        return MIP_OK;
    }
    if (dev->flags & NETIF_FLAG_UP)
    {
        los_mip_netif_down(dev);
    }
    before = NULL;
    cur = g_netif_list;
    while (cur != NULL)
    {
        if (dev == cur)
        {
            if (before == NULL)
            {
                g_netif_list = cur->next;
            }
            else
            {
                before->next = cur->next;
            }
            break;
        }
        before = cur;
        cur = cur->next;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_netif_config
 Description : config the network device's ip netmask gateway ...
 Input       : dev @ net dev's handle which need do config 
               ipaddr @ ip address set to dev
               netmask @ netmask set to dev
               gw @ gateway set to dev
 Output      : None
 Return      : MIP_OK config dev ok.
 *****************************************************************************/
int los_mip_netif_config( struct netif *dev, 
                        ip_addr_t *ipaddr, 
                        ip_addr_t *netmask,
                        ip_addr_t *gw )
{
    if (NULL == dev)
    {
        return MIP_OK;
    }
    if (NULL != ipaddr)
    {
        memcpy(&dev->ip_addr, ipaddr, sizeof(ip_addr_t));
    }
    if (NULL != netmask)
    {
        memcpy(&dev->netmask, netmask, sizeof(ip_addr_t));
    }
    if (NULL != gw)
    {
        memcpy(&dev->gw, gw, sizeof(ip_addr_t));
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_netif_up
 Description : start up network device.
 Input       : dev @ net dev's handle which need start up 
 Output      : None
 Return      : MIP_OK start up dev ok.
 *****************************************************************************/
int los_mip_netif_up(struct netif *dev)
{
    if (NULL == dev)
    {
        return MIP_OK;
    }
    los_mip_netif_add(dev);
    
    /* to do the up actions */
    dev->flags |= NETIF_FLAG_UP;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_netif_down
 Description : shut down network device.
 Input       : dev @ net dev's handle which need shut down
 Output      : None
 Return      : MIP_OK shut down dev ok.
 *****************************************************************************/
int los_mip_netif_down(struct netif *dev)
{
    if (NULL == dev)
    {
        return MIP_OK;
    }
    dev->flags &= ~NETIF_FLAG_UP;
    return MIP_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
