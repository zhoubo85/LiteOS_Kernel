#include <stdlib.h>
#include <string.h>
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"
#include "los_mip_netif.h"
#include "los_mip_tcpip_core.h"


struct netif *g_netif_list = NULL;
struct netif *g_if_default = NULL;

struct netif *los_mip_get_netif_list(void)
{
    return g_netif_list;
}

void los_mip_set_default_if(struct netif *dev)
{
    g_if_default = dev;
}
struct netif * los_mip_get_default_if(void)
{
    return g_if_default;
}
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

int los_mip_netif_down(struct netif *dev)
{
    if (NULL == dev)
    {
        return MIP_OK;
    }
    return MIP_OK;
}
