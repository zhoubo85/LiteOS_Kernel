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
#include "los_mip_igmp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#if MIP_EN_IGMP

/* all igmp group ip 224.0.0.1 */
static struct ipv4_addr igmp_allgroup;
/* all igmp router ip 224.0.0.2 */
static struct ipv4_addr igmp_allroute;



/*****************************************************************************
 Function    : los_mip_igmp_malloc_group
 Description : malloc buffer for a new igmp group.
 Input       : addr @ the igmp group address 
 Output      : None
 Return      : tmp @ the igmp group's pointer, if failed return NULL 
 *****************************************************************************/
static struct mip_igmp_group *los_mip_igmp_malloc_group(struct ipv4_addr *addr) 
{
    struct mip_igmp_group *tmp = NULL;
    if (NULL == addr)
    {
        return NULL;
    }
    tmp = (struct mip_igmp_group *)los_mpools_malloc(MPOOL_MSGS, 
                                                sizeof(struct mip_igmp_group));
    if (tmp)
    {
        tmp->address.addr = addr->addr;
        tmp->next = NULL;
        tmp->ref = 0;
        tmp->state = IGMP_STATE_NONE;
        tmp->timer = MIP_INVALID_TIMER;
    }
    
    return tmp;
}

/*****************************************************************************
 Function    : los_mip_igmp_free_group
 Description : free igmp group's buffer and other resources
 Input       : group @ the igmp group that need free 
 Output      : None
 Return      : tmp @ the igmp group's pointer, if failed return NULL 
 *****************************************************************************/
static int los_mip_igmp_free_group(struct mip_igmp_group *group) 
{
    if (NULL == group)
    {
        return MIP_OK;
    }
    if (los_mip_is_valid_timer(group->timer))
    {
        los_mip_stop_timer(group->timer);
        los_mip_delete_timer(group->timer);
        los_mpools_free(MPOOL_MSGS, group);
        group = NULL;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_remove_group
 Description : remove group from dev's igmp group list 
 Input       : dev @ the net interface's pointer
               grp @ the igmp group that need remove from the device.
 Output      : None
 Return      : MIP_OK @ remove group success, other value means failed
 *****************************************************************************/
static int los_mip_igmp_remove_group(struct netif *dev,
                                     struct mip_igmp_group *grp) 
{
    struct mip_igmp_group *last = NULL;
    struct mip_igmp_group *cur = NULL;
    if ((NULL == dev) || (NULL == grp))
    {
        return -MIP_ERR_PARAM;
    }
    cur = (struct mip_igmp_group *)dev->igmp;
    while(NULL != cur)
    {
        if (cur == grp)
        {
            if (NULL == last)
            {
                dev->igmp = (void *)cur->next;
            }
            else
            {
                last->next = cur->next;
            }
            /* free group's buffer */
            los_mip_igmp_free_group(grp);
            break;
        }
        last = cur;
        cur = cur->next;
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_add_group
 Description : add a new igmp group to the dev's igmp group list.
 Input       : dev @ the net interface's pointer
               grp @ the igmp group that will added
 Output      : None
 Return      : MIP_OK @ add group success, other value means failed
 *****************************************************************************/
static int los_mip_igmp_add_group(struct netif *dev,
                                  struct mip_igmp_group *grp) 
{
    struct mip_igmp_group *cur = NULL;
    if ((NULL == dev) || (NULL == grp))
    {
        return -MIP_ERR_PARAM;
    }

    cur = (struct mip_igmp_group *)dev->igmp;
    if (NULL == cur)
    {
        dev->igmp = (void *)grp;
        return MIP_OK;
    }
    while(NULL != cur->next)
    {
        cur = cur->next;
    }
    cur->next = grp;
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_find_group
 Description : find a igmp group from the dev's igmp group list.
 Input       : dev @ the net interface's pointer
               addr @ multicast ip address that want to be found in the 
               dev's igmp group list.
 Output      : None
Return       : tmp @ the group's pointer, if not find, return NULL 
 *****************************************************************************/
struct mip_igmp_group *los_mip_igmp_find_group(struct netif *dev, 
                                                struct ipv4_addr *addr)
{
    struct mip_igmp_group *tmp = NULL;
    if ((NULL == dev) || (NULL == addr))
    {
        return NULL;
    }
    tmp = (struct mip_igmp_group *)dev->igmp;
    while(NULL != tmp)
    {
        if (tmp->address.addr == addr->addr)
        {
            return tmp;
        }
    }
    return NULL;
}

/*****************************************************************************
 Function    : los_mip_igmp_init
 Description : enable the multicast function on the given interface, and init
               the igmp group.
 Input       : dev @ the net interface's pointer
               group @ igmp group pointer.
 Output      : None
 Return      : dev @ device's pointer which contain the igmp group, 
 *****************************************************************************/
int los_mip_igmp_init(struct netif *dev)
{
    struct mip_igmp_group *newgrp = NULL;
    igmp_allgroup.addr = IGMP_ALL_GROUP;
    igmp_allroute.addr = IGMP_ALL_ROUTER;
    
    /* add the default support multicast group 224.0.0.1 */
    newgrp = los_mip_igmp_malloc_group(&igmp_allgroup);
    if (newgrp)
    {
        los_mip_igmp_add_group(dev, newgrp);
        if (dev->igmp_config)
        {
            /* call dev's igmp address config function to add igmp mac filter */
            dev->igmp_config(dev, &igmp_allgroup, MIP_ADD_MAC_FILTER);
        }
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_send
 Description : create a igmp message and send it out. the message's type will 
               be config by the type param.
 Input       : group @ igmp group pointer.
 Output      : None
 Return      : dev @ device's pointer which contain the igmp group, 
 *****************************************************************************/
int los_mip_igmp_send(struct netif *dev, struct mip_igmp_group *group, u8_t type)
{
    struct netbuf *p = NULL;
    int ret = MIP_OK;
    struct igmp_msgv12 *igmpmsg = NULL;
    struct ipv4_addr *dst = NULL;
    struct ipv4_addr *src = NULL;
    u16_t opt[2];
    opt[0] = MIP_HTONS(RALERT);
    opt[1] = 0x0000; /* Router shall examine packet */
    
    if ((NULL == dev) || (NULL == group))
    {
        return -MIP_ERR_PARAM;
    }
    p = netbuf_alloc(NETBUF_TRANS, IGMP_PKG_V12_SIZE);
    if (p)
    {
        memset(p->payload, 0, IGMP_PKG_V12_SIZE);
        igmpmsg = (struct igmp_msgv12 *)p->payload;
        src = &dev->ip_addr;
        memcpy((void *)&igmpmsg->group_addr.addr, (void *)&group->address.addr, 
               sizeof(struct ipv4_addr));
        switch(type)
        {
            case IGMP_MEMB_REPORT2:
                dst = &group->address;
                break;
            case IGMP_MEMB_LEAVE:
                dst = &igmp_allroute;
                break;
            default:
                break;
        }
        if (dst)
        {
            /* correct message type */
            igmpmsg->type = type;
            igmpmsg->checksum = 0;
            igmpmsg->reserved = 0;
            igmpmsg->checksum = los_mip_checksum(p->payload, p->len);
            ret = los_mip_ipv4_output(dev, p, src, dst,
                                      IGMP_TTL, 0, MIP_PRT_IGMP, 
                                      (u8_t *)&opt, RALERT_LEN);
        }
        else
        {
            /* unknown message type, do not send message */
            netbuf_free(p);
        }
    }
    else
    {
        /* malloc buffer failed */
        ret = -MIP_MPOOL_MALLOC_FAILED;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_igmp_get_dev
 Description : find which dev has the igmp group info
 Input       : group @ igmp group pointer.
 Output      : None
 Return      : dev @ device's pointer which contain the igmp group, 
 *****************************************************************************/
static struct netif *los_mip_igmp_get_dev(struct mip_igmp_group *group)
{
    struct netif *dev = NULL;
    struct mip_igmp_group *tmp = NULL;
    if (NULL == group)
    {
        return NULL;
    }
    
    dev = los_mip_get_netif_list();
    while(NULL != dev)
    {
        tmp = dev->igmp;
        while(NULL != tmp)
        {
            if (tmp == group)
            {
                return dev;
            }
            tmp = tmp->next;
        }
        dev = dev->next;
    }
    return dev;
}

/*****************************************************************************
 Function    : los_mip_igmp_timeout
 Description : igmp delayed timer timeout callback function .
 Input       : arg @ the igmp group that need to be process
 Output      : None
 Return      : None.
 *****************************************************************************/
static void los_mip_igmp_timeout(u32_t arg)
{
    struct netif *dev = NULL;
    
    struct mip_igmp_group *group = (struct mip_igmp_group *)arg;
    
    dev = los_mip_igmp_get_dev(group);
    if (NULL != dev)
    {
        /* do send igmp reply message and stop the timer */
        los_mip_igmp_send(dev, group, IGMP_MEMB_REPORT2);
        los_mip_stop_timer(group->timer);
    }
    group->state = IGMP_STATE_IDLE;
    return ;
}

/*****************************************************************************
 Function    : los_mip_start_igmp_delay_timer
 Description : start igmp group reply delay timer .
 Input       : group @ the igmp group that need to start the timer
 Output      : None
 Return      : ret @ MIP_OK means start ok, other means error happend.
 *****************************************************************************/
int los_mip_start_igmp_delay_timer(struct mip_igmp_group *group)
{
    int ret = MIP_OK;
    if (NULL == group)
    {
        return -MIP_IGMP_TMR_START_ERR;
    }
    if (!los_mip_is_valid_timer(group->timer))
    {
        group->timer = los_mip_create_swtimer(IGMP_DEFAULT_DELAY_TIMEOUT, 
                                              los_mip_igmp_timeout, 
                                              (u32_t)group); 
    }
    if (IGMP_STATE_TMR_RUNNING != group->state)
    {
        if(MIP_FALSE == los_mip_start_timer(group->timer))
        {
            ret = -MIP_IGMP_TMR_START_ERR;
        }
        group->state = IGMP_STATE_TMR_RUNNING;
    }
    return ret;
}

/*****************************************************************************
 Function    : los_mip_get_igmp_groupref
 Description : get igmp group pointer by the net interface .
 Input       : dev @ net dev's handle which the data come from
 Output      : None
 Return      : dev->igmp @ if ok igmp group pointer , failed return NULL.
 *****************************************************************************/
struct mip_igmp_group *los_mip_get_igmp_groupref(struct netif *dev)
{
    if (NULL == dev || NULL == dev->igmp)
    {
        return NULL;
    }
    return (struct mip_igmp_group *)dev->igmp;
}

/*****************************************************************************
 Function    : los_mip_find_fixed_group
 Description : find igmp group pointer by a given address .
 Input       : group @ igmp group list 
               addr @ the ip address of a specified igmp group
 Output      : None
 Return      : dev->igmp @ if ok igmp group pointer , failed return NULL.
 *****************************************************************************/
struct mip_igmp_group *los_mip_find_fixed_group(struct mip_igmp_group *group, 
                                                struct ipv4_addr *addr)
{
    struct mip_igmp_group *tmp = NULL;
    if (NULL == group || NULL == addr)
    {
        return NULL;
    }
    tmp = group;
    while(NULL != tmp)
    {
        if (tmp->address.addr == addr->addr)
        {
            break;
        }
        tmp = tmp->next;
    }
    return tmp;
}

/*****************************************************************************
 Function    : los_mip_igmp_process_query
 Description : process igmp query message, and make some respone 
               if need.
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               src @ source ip address of the package
               dst @ dest ip address of the package.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_igmp_process_query(struct netbuf *p,
                               struct netif *dev, 
                               ip_addr_t *src, 
                               ip_addr_t *dst)
{
    struct igmp_msgv12 *igmpmsg = NULL;
    struct mip_igmp_group *groupref = NULL;
    struct mip_igmp_group *onegroup = NULL;
    
    igmpmsg = (struct igmp_msgv12 *)p->payload;
    groupref = los_mip_get_igmp_groupref(dev);
    if (NULL != groupref)
    {
        /* skip the first group 224.0.0.1 */
        groupref = groupref->next;
    }
    if (los_mip_ip_equal(dst, &igmp_allgroup) 
        && los_mip_ip_addrany(&igmpmsg->group_addr))
    {
        /* message for query ip 224.0.0.1(all group address) */
        while(NULL != groupref)
        {
            /* start igmp delay timer to do reply function */
            los_mip_start_igmp_delay_timer(groupref);
            groupref = groupref->next;
        }
    }
    else
    {
        if (!los_mip_ip_addrany(&igmpmsg->group_addr))
        {
            /* this message is for query a specific group */
            //first find the group
            if (los_mip_ip_equal(dst, &igmp_allgroup))
            {
                onegroup = los_mip_find_fixed_group(groupref, 
                                                    &igmpmsg->group_addr);
            }
            los_mip_start_igmp_delay_timer(onegroup);
        }
        else
        {
            /* error happend */
        }
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_process_report
 Description : process igmp report message, if the report group is the same as 
               delay timer processing group, we need stop the timer.
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               src @ source ip address of the package
               dst @ dest ip address of the package.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_igmp_process_report(struct netbuf *p,
                               struct netif *dev, 
                               ip_addr_t *src, 
                               ip_addr_t *dst)
{
    struct mip_igmp_group *groupref = NULL;
    struct mip_igmp_group *onegroup = NULL;
    
    groupref = los_mip_get_igmp_groupref(dev);
    if (NULL != groupref)
    {
        /* skip the first group 224.0.0.1 */
        groupref = groupref->next;
    }
    onegroup = los_mip_find_fixed_group(groupref, dst);
    if (onegroup 
        && (IGMP_STATE_TMR_RUNNING == onegroup->state))
    {
        los_mip_stop_timer(onegroup->timer);
        onegroup->state = IGMP_STATE_IDLE;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_input
 Description : parse igmp input package 
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               src @ source ip address of the package
               dst @ dest ip address of the package.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_igmp_input(struct netbuf *p, struct netif *dev, 
                       ip_addr_t *src, ip_addr_t *dst)
{
    struct igmp_msgv12 *igmpmsg = NULL;
    u32_t checksum = 0;
    int ret = MIP_OK;
    if ((NULL == p) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    if (p->len != IGMP_PKG_V12_SIZE)
    {
        /* now we just support v1 and v2, v3 not support yet */
        netbuf_free(p);
        return -MIP_IGMP_PKG_LEN_ERR;
    }
    igmpmsg = (struct igmp_msgv12 *)p->payload;
    checksum = los_mip_checksum(p->payload, p->len);
    if (checksum)
    {
        /* check sum errror */
        netbuf_free(p);
        return -MIP_IGMP_PKG_CHKSUM_ERR;
    }
    
    switch(igmpmsg->type)
    {
        case IGMP_MEMB_QUERY:
            ret = los_mip_igmp_process_query(p, dev, src, dst);
            break;
        case IGMP_MEMB_REPORT1:
        case IGMP_MEMB_REPORT2:
            ret = los_mip_igmp_process_report(p, dev, src, dst);
            break;
        case IGMP_MEMB_LEAVE:
            break;
        default :
            break;
    }
    /* free the igmp package received */
    netbuf_free(p);
    p = NULL;
    return ret;
}

/*****************************************************************************
 Function    : los_mip_igmp_join_group
 Description : join a group on the net interface 
 Input       : ipaddr @ ip address of the network interface which s
               hould join a new group
               groupaddr @ the group that will be jion.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_igmp_join_group(ip_addr_t *ipaddr, ip_addr_t *groupaddr)
{
    struct netif *dev = NULL;
    struct mip_igmp_group *tmpgrp = NULL;
    int ret = MIP_OK;
    u8_t onedev = 1;
    
    if ((NULL == ipaddr) || (NULL == groupaddr))
    {
        return -MIP_ERR_PARAM;
    }
    dev = los_mip_get_netif_list();
    //los_mip_ip_equal(dst, &igmp_allgroup)
    if(los_mip_ip_addrany(ipaddr))
    {
        /* the group will add for all dev */
        onedev = 0;
    }
    else
    {
        /* just for a given net interface */
    }
    while (NULL != dev)
    {
        if (onedev)
        {
            if (!los_mip_ip_equal(&dev->ip_addr, ipaddr))
            {
                continue;
            }
        }
        tmpgrp = los_mip_igmp_find_group(dev, groupaddr);
        if (NULL == tmpgrp)
        {
            /* create a new gropu and join it to the group list */
            tmpgrp = los_mip_igmp_malloc_group(groupaddr);
            ret = los_mip_igmp_add_group(dev, tmpgrp);
            if (ret == MIP_OK)
            {
                if (dev->igmp_config)
                {
                    dev->igmp_config(dev, groupaddr, MIP_ADD_MAC_FILTER);
                }
                /* start delay timer tell router dev join a group */
                los_mip_start_igmp_delay_timer(tmpgrp);
            }
        }
        else
        {
            tmpgrp->ref++;
        }
        dev = dev->next;
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_igmp_leave_group
 Description : leave a group on net interface. 
 Input       : ipaddr @ ip address of the network interface which 
               should leave a new group
               groupaddr @ the group that will be jion.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_igmp_leave_group(ip_addr_t *ipaddr, ip_addr_t *groupaddr)
{
    struct netif *dev = NULL;
    struct mip_igmp_group *tmpgrp = NULL;
    int ret = MIP_OK;
    u8_t onedev = 1;
    
    if ((NULL == ipaddr) || (NULL == groupaddr))
    {
        return -MIP_ERR_PARAM;
    }
    dev = los_mip_get_netif_list();
    if(los_mip_ip_addrany(ipaddr))
    {
        /* the group will add for all dev */
        onedev = 0;
    }
    else
    {
        /* just for a given net interface */
    }
    while (NULL != dev)
    {
        if (onedev)
        {
            if (!los_mip_ip_equal(&dev->ip_addr, ipaddr))
            {
                continue;
            }
        }
        tmpgrp = los_mip_igmp_find_group(dev, groupaddr);
        if (NULL != tmpgrp)
        {
            if (tmpgrp->ref <= 1)
            {
                los_mip_igmp_send(dev, tmpgrp, IGMP_MEMB_LEAVE);
                ret = los_mip_igmp_remove_group(dev, tmpgrp);
            }
            else
            {
                /* multi task use the same igmp group */
                tmpgrp->ref--;
            }
            if (dev->igmp_config)
            {
                dev->igmp_config(dev, groupaddr, MIP_DEL_MAC_FILTER);
            }
        }
        dev = dev->next;
    }
    return ret;
}

#endif /* MIP_EN_IGMP */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
