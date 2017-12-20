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

#include "los_mip_dhcp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* dhcp option buf size */
#define MIP_OPT_TMP_LEN 16

/* dhcp transmit id */
static u32_t g_rand_tranid = 0x12657897;
/* dhcp option buf */
static u8_t g_opt_tmp[MIP_OPT_TMP_LEN] = {0};
/* dhcp funciton address ip/netmask/gateway ... */
static u32_t g_requested_ip = 0;
static u32_t g_requested_netmask = 0;
static u32_t g_requested_gw = 0;
static u32_t g_requested_dhcpser = 0;
/* dhcp task id */
static mip_thread_t g_dhcp_task_id;
static dhcp_callback g_dhcp_callback = NULL;

/*****************************************************************************
 Function    : los_mip_new_dhcp_buf
 Description : malloc a dhcp message buf
 Input       : exoptlen @ external option value's buf length, default is 0
 Output      : None
 Return      : netbuf * @ dhcp payload buf's pointer
 *****************************************************************************/
struct netbuf *los_mip_new_dhcp_buf(u16_t exoptlen)
{
    struct netbuf *pbuf = NULL;
    
    pbuf = netbuf_alloc(NETBUF_TRANS, exoptlen + sizeof(struct dhcp_ipv4_info));

    return pbuf;
}

/*****************************************************************************
 Function    : los_mip_init_dhcp_msg
 Description : init dhcp request message data in netbuf
 Input       : pbuf @ the buf that store dhcp message
               dev @ the device handle that message will send out.
 Output      : None
 Return      : MIP_OK init message success. other value means failed.
 *****************************************************************************/
static int los_mip_init_dhcp_msg(struct netbuf *pbuf, struct netif *dev)
{
    struct dhcp_ipv4_info *dhcpdata = NULL;
    
    if (NULL == pbuf)
    {
        return -MIP_ERR_PARAM;
    }
    dhcpdata = (struct dhcp_ipv4_info *)pbuf->payload;
    dhcpdata->opcode = MIP_DHCP_OP_REQUEST;
    dhcpdata->hwtype = MIP_DHCP_HW_ETH;
    dhcpdata->hwlen = MIP_DHCP_HW_ETH_LEN;
    dhcpdata->transid = g_rand_tranid;
    dhcpdata->second = 0x0;
    dhcpdata->flags = 0x0;
    dhcpdata->clientip = 0x0;
    dhcpdata->yourip = 0x0;
    dhcpdata->serverip = 0x0;
    dhcpdata->gwip = 0x0;
    memcpy(dhcpdata->clienmac, dev->hwaddr, MIP_DHCP_HW_ETH_LEN);
    dhcpdata->magiccookie = MIP_DHCP_MAGIC_COOKIE;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_set_option
 Description : add an option to dhcp message
 Input       : hdr @ dhcp message's pointer
               type @ option type
               val @ option value's pointer
               fisrtopt @ indicate if the option placed in hdr->options[0]
 Output      : None
 Return      : MIP_OK set option ok, other value means failed.
 *****************************************************************************/
static int los_mip_dhcp_set_option(struct dhcp_ipv4_info *hdr, 
                                   u8_t type, u8_t len,
                                   u8_t *val, u8_t fisrtopt)
{
    static u16_t optpos = 0;
    
    if (NULL == hdr)
    {
        return -MIP_ERR_PARAM;
    }
    if (fisrtopt)
    {
        optpos = 0;
    }
    hdr->options[optpos++] = type;
    if (type == MIP_DHCP_OPT_END)
    {
        optpos = 0;
        return MIP_OK;
    }
    if (NULL == val)
    {
        return -MIP_ERR_PARAM;
    }
    hdr->options[optpos++] = len;
    memcpy(hdr->options + optpos, val, len);
    optpos += len;
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_discover
 Description : init dhcp discover message
 Input       : dev @ device pointer which the message will be send.
 Output      : None
 Return      : netbuf * @ dhcp discover message's buf pointer.
 *****************************************************************************/
struct netbuf * los_mip_dhcp_discover(struct netif *dev)
{
    struct netbuf *pbuf = NULL;
    struct dhcp_ipv4_info *dhcpdata = NULL;
    u8_t optval = 0;
    
    pbuf = los_mip_new_dhcp_buf(0);
    if (NULL == pbuf)
    {
        return NULL;
    }
    dhcpdata = (struct dhcp_ipv4_info *)pbuf->payload;
    los_mip_init_dhcp_msg(pbuf, dev);
    optval = MIP_DHCP_DISCOVER;
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_MSG_TYPE, 
                            MIP_DHCP_OPT_MSG_TYPE_LEN,
                            &optval, 1);
    
    g_opt_tmp[0] = MIP_DHCP_HW_ETH;
    memcpy(g_opt_tmp+1, dev->hwaddr, MIP_ETH_HW_LEN);
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_CLIENT_ID, 
                            MIP_DHCP_OPT_CLIENT_ID_ETH_LEN,
                            g_opt_tmp, 0);
    
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    g_opt_tmp[0] = MIP_DHCP_OPT_SUBNET_MASK;
    g_opt_tmp[1] = MIP_DHCP_OPT_ROUTER;
    g_opt_tmp[2] = MIP_DHCP_OPT_BROADCAST;
    g_opt_tmp[3] = MIP_DHCP_OPT_DNS_SERVER;
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_PARAMETER_REQUEST_LIST, 
                            MIP_DHCP_OPT_PARAMETER_REQUEST_LIST_LEN,
                            g_opt_tmp, 0);
    
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_END, 0,
                            NULL, 0);
                            
    return pbuf;
}

/*****************************************************************************
 Function    : los_mip_dhcp_request
 Description : init dhcp request message
 Input       : dev @ device pointer which the message will be send.
 Output      : None
 Return      : netbuf * @ dhcp request message's buf pointer.
 *****************************************************************************/
struct netbuf * los_mip_dhcp_request(struct netif *dev)
{
    struct netbuf *pbuf = NULL;
    struct dhcp_ipv4_info *dhcpdata = NULL;
    u8_t optval = 0;
    
    if (NULL == dev)
    {
        return NULL;
    }
    pbuf = los_mip_new_dhcp_buf(0);
    if (NULL == pbuf)
    {
        return NULL;
    }
    dhcpdata = (struct dhcp_ipv4_info *)pbuf->payload;
    los_mip_init_dhcp_msg(pbuf, dev);
    
    optval = MIP_DHCP_REQUEST;
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_MSG_TYPE, 
                            MIP_DHCP_OPT_MSG_TYPE_LEN,
                            &optval, 1);
    
    g_opt_tmp[0] = MIP_DHCP_HW_ETH;
    memcpy(g_opt_tmp+1, dev->hwaddr, MIP_ETH_HW_LEN);
    los_mip_dhcp_set_option(dhcpdata, 
                            MIP_DHCP_OPT_CLIENT_ID, 
                            MIP_DHCP_OPT_CLIENT_ID_ETH_LEN,
                            g_opt_tmp, 0);
                            
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    memcpy(g_opt_tmp, &g_requested_ip, MIP_DHCP_OPT_REQUESTED_IPV4_LEN);
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_REQUESTED_IP,
                            MIP_DHCP_OPT_REQUESTED_IPV4_LEN,
                            g_opt_tmp, 0);
                            
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    memcpy(g_opt_tmp, &g_requested_dhcpser, MIP_DHCP_OPT_REQUESTED_IPV4_LEN);
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_SERVER_ID, 
                            MIP_DHCP_OPT_REQUESTED_IPV4_LEN,
                            g_opt_tmp, 0);                      
    
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    g_opt_tmp[0] = MIP_DHCP_OPT_SUBNET_MASK;
    g_opt_tmp[1] = MIP_DHCP_OPT_ROUTER;
    g_opt_tmp[2] = MIP_DHCP_OPT_BROADCAST;
    g_opt_tmp[3] = MIP_DHCP_OPT_DNS_SERVER;
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_PARAMETER_REQUEST_LIST, 
                            MIP_DHCP_OPT_PARAMETER_REQUEST_LIST_LEN,
                            g_opt_tmp, 0);
                            
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_END, 0,
                            NULL, 0);
    
    return pbuf;
}

/*****************************************************************************
 Function    : los_mip_dhcp_decline
 Description : init dhcp decline message
 Input       : dev @ device pointer which the message will be send.
 Output      : None
 Return      : netbuf * @ dhcp decline message's buf pointer.
 *****************************************************************************/
struct netbuf * los_mip_dhcp_decline(struct netif *dev)
{
    struct netbuf *pbuf = NULL;
    struct dhcp_ipv4_info *dhcpdata = NULL;
    u8_t optval = 0;
    
    pbuf = los_mip_new_dhcp_buf(0);
    if (NULL == pbuf)
    {
        return NULL;
    }
    dhcpdata = (struct dhcp_ipv4_info *)pbuf->payload;
    los_mip_init_dhcp_msg(pbuf, dev);
    
    optval = MIP_DHCP_DECLINE;
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_MSG_TYPE, 
                            MIP_DHCP_OPT_MSG_TYPE_LEN,
                            &optval, 1);
    
    g_opt_tmp[0] = MIP_DHCP_HW_ETH;
    memcpy(g_opt_tmp+1, dev->hwaddr, MIP_ETH_HW_LEN);
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_CLIENT_ID, 
                            MIP_DHCP_OPT_CLIENT_ID_ETH_LEN,
                            g_opt_tmp, 0);
                            
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    memcpy(g_opt_tmp, &g_requested_ip, MIP_DHCP_OPT_REQUESTED_IPV4_LEN);
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_REQUESTED_IP, 
                            MIP_DHCP_OPT_REQUESTED_IPV4_LEN,
                            g_opt_tmp, 0);
                            
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    memcpy(g_opt_tmp, &g_requested_dhcpser, MIP_DHCP_OPT_REQUESTED_IPV4_LEN);
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_SERVER_ID, 
                            MIP_DHCP_OPT_REQUESTED_IPV4_LEN,
                            g_opt_tmp, 0);                      
    
    memset(g_opt_tmp, 0, MIP_OPT_TMP_LEN);
    g_opt_tmp[0] = MIP_DHCP_OPT_SUBNET_MASK;
    g_opt_tmp[1] = MIP_DHCP_OPT_ROUTER;
    g_opt_tmp[2] = MIP_DHCP_OPT_BROADCAST;
    g_opt_tmp[3] = MIP_DHCP_OPT_DNS_SERVER;
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_PARAMETER_REQUEST_LIST, 
                            MIP_DHCP_OPT_PARAMETER_REQUEST_LIST_LEN,
                            g_opt_tmp, 0);
                            
    los_mip_dhcp_set_option(dhcpdata, MIP_DHCP_OPT_END, 0,
                            NULL, 0);
    
    return pbuf;
}

/*****************************************************************************
 Function    : los_mip_dhcp_parse_option
 Description : parse dhcp option and get the option and its value.
 Input       : buf @ dhcp buf's pointer.
               len @ data length of the buf
 Output      : None
 Return      : MIP_OK means process ok, other value means failed.
 *****************************************************************************/
int los_mip_dhcp_parse_option(u8_t *buf, int len)
{
    u8_t opt = 0;
    int optlen = 0;
    
    if (NULL == buf)
    {
        return -MIP_ERR_PARAM;
    }
    opt = buf[0];
    optlen = buf[1];
    while ((opt != MIP_DHCP_OPT_END) && (len > 0))
    {
        switch (opt)
        {
            case MIP_DHCP_OPT_SUBNET_MASK:
                memcpy(&g_requested_netmask, buf+2, optlen);
                break;
            case MIP_DHCP_OPT_ROUTER:
                memcpy(&g_requested_gw, buf+2, optlen);
                break;
            case MIP_DHCP_OPT_SERVER_ID:
                memcpy(&g_requested_dhcpser, buf+2, optlen);
                break;
            default:
                break;
        }
        len = len - optlen - 2;
        buf = buf + optlen + 2;
        opt = buf[0];
        optlen = buf[1];
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_process_offer
 Description : parse dhcp offer message and get the ip data in the message.
 Input       : p @ dhcp offer buf's pointer.
               dev @ device's handle point witch the message comes up.
 Output      : None
 Return      : MIP_OK means process ok, other value means failed.
 *****************************************************************************/
int los_mip_dhcp_process_offer(struct netbuf *p, struct netif *dev)
{
    struct dhcp_ipv4_info *dhcpdata = NULL;
    
    dhcpdata = (struct dhcp_ipv4_info *)p->payload;
    if (dhcpdata->opcode != MIP_DHCP_OP_REPLY)
    {
        return MIP_FALSE;
    }
    if ((dhcpdata->options[0] != MIP_DHCP_OPT_MSG_TYPE) 
        || (dhcpdata->options[2] != MIP_DHCP_OFFER))
    {
        return MIP_FALSE;
    }
    g_requested_ip = dhcpdata->yourip;
    los_mip_dhcp_parse_option(dhcpdata->options, p->len);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_process_offer
 Description : parse dhcp ack message and get the ip data in the message.
 Input       : p @ dhcp offer buf's pointer.
               dev @ device's handle point witch the message comes up.
 Output      : None
 Return      : MIP_OK means process ok, other value means failed.
 *****************************************************************************/
int los_mip_dhcp_process_ack(struct netbuf *p, struct netif *dev)
{
    struct dhcp_ipv4_info *dhcpdata = NULL;
    
    dhcpdata = (struct dhcp_ipv4_info *)p->payload;
    if (dhcpdata->opcode != MIP_DHCP_OP_REPLY)
    {
        return MIP_FALSE;
    }
    if ((dhcpdata->options[0] != MIP_DHCP_OPT_MSG_TYPE) 
        || (dhcpdata->options[2] != MIP_DHCP_ACK))
    {
        return MIP_FALSE;
    }
    g_requested_ip = dhcpdata->yourip;
    los_mip_dhcp_parse_option(dhcpdata->options, p->len);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_task
 Description : dhcp process task, all dhcp state will processed in the task
 Input       : argument @ dhcp offer buf's pointer.
 Output      : None
 Return      : None.
 *****************************************************************************/
void los_mip_dhcp_task(void * argument)
{
    struct netif *dev = (struct netif *) argument;
    struct mip_conn *con = NULL;
    struct sockaddr_in localaddr;
    u8_t dhcp_retry = 0;
    u8_t dhcp_stage = MIP_DHCP_DISCOVER;
    int ret = MIP_OK;
    struct skt_rcvd_msg *msgs = NULL;
    struct netbuf *buf = NULL;
    ip_addr_t tmp_ipaddr;
    
    if (NULL == dev)
    {
        return ;
    }
    
    memset(&dev->ip_addr, 0, sizeof(ip_addr_t));
    memset(&dev->netmask, 0, sizeof(ip_addr_t));
    memset(&dev->gw, 0, sizeof(ip_addr_t));
    
    dev->autoipflag = AUTO_IP_PROCESSING;
    
    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localaddr.sin_port = htons(MIP_DHCP_CLI_PORT);
    
    con = los_mip_new_conn(CONN_UDP);
    if (NULL == con)
    {
        return;
    }
    /* recive func just wait for 5 scends */
    con->recv_timeout = 5000;
    los_mip_udp_bind(con->ctlb.udp, 
                     &localaddr.sin_addr.s_addr, 
                     localaddr.sin_port); 
    
    while (1)
    {
        if (dhcp_retry >= MIP_DHCP_MAX_RETRY)
        {
            /* tried MIP_DHCP_MAX_RETRY times, so we don't try again, stop dhcp */ 
            if (NULL != g_dhcp_callback)
            {
                g_dhcp_callback(MIP_DHCP_ERR_TIMEOUT, MIP_DHCP_STATE_FINISHED);
            }
            break;
        }
        switch (dhcp_stage)
        {
            case MIP_DHCP_DISCOVER:
                /* todo: send discover messages */
                g_rand_tranid = los_mip_random();
                buf = los_mip_dhcp_discover(dev);
            
                con->ctlb.udp->remote_ip.addr = MIP_HTONL(MIP_IP_BROADCAST);
                con->ctlb.udp->rport = MIP_HTONS(MIP_DHCP_SER_PORT);
                con->ctlb.udp->ttl = MIP_IP_DEFAULT_TTL;
                con->ctlb.udp->tos = 0;
                if (NULL != g_dhcp_callback)
                {
                    g_dhcp_callback(MIP_DHCP_ERR_NONE, MIP_DHCP_STATE_PROCESSING);
                }
                ret = los_mip_snd_sktmsg(con, buf); 
                if (ret != MIP_OK)
                {
                    if (NULL != g_dhcp_callback)
                    {
                        g_dhcp_callback(MIP_DHCP_ERR_NONE, MIP_DHCP_STATE_PROCESSING);
                    }
                    dhcp_retry++;
                    netbuf_free(buf);
                    break;
                }
                dhcp_stage = MIP_DHCP_OFFER;
                break;
            case MIP_DHCP_OFFER:
                /* 
                    wait for server give offer
                    todo: need recive messages 
                */
                if (NULL != g_dhcp_callback)
                {
                    g_dhcp_callback(MIP_DHCP_ERR_NONE, MIP_DHCP_STATE_PROCESSING);
                }
                ret = los_mip_mbox_fetch(&con->recvmbox, 
                                         (void **)&msgs, 
                                          con->recv_timeout);
                if (MIP_OK != ret)
                {
                    dhcp_retry++;
                    dhcp_stage = MIP_DHCP_DISCOVER;
                    break;
                }
                ret = los_mip_dhcp_process_offer(msgs->p, dev);
                netbuf_free(msgs->p);
                msgs->p = NULL;
                los_mip_delete_up_sktmsg(msgs);
                if (MIP_OK != ret)
                {
                    dhcp_retry++;
                    dhcp_stage = MIP_DHCP_DISCOVER;
                    break;
                }
                dhcp_stage = MIP_DHCP_REQUEST;
                break;
            case MIP_DHCP_REQUEST:
                if (NULL != g_dhcp_callback)
                {
                    g_dhcp_callback(MIP_DHCP_ERR_NONE, MIP_DHCP_STATE_PROCESSING);
                }
                buf = los_mip_dhcp_request(dev);
                
                ret = los_mip_snd_sktmsg(con, buf); 
                if (ret != MIP_OK)
                {
                    dhcp_retry++;
                    netbuf_free(buf);
                    break;
                }
                dhcp_stage = MIP_DHCP_ACK;
                break;
            case MIP_DHCP_ACK:
                /* wait for server give ack */
                if (NULL != g_dhcp_callback)
                {
                    g_dhcp_callback(MIP_DHCP_ERR_NONE, MIP_DHCP_STATE_PROCESSING);
                }
                ret = los_mip_mbox_fetch(&con->recvmbox, (void **)&msgs, con->recv_timeout);
                if (MIP_OK != ret)
                {
                    dhcp_retry++;
                    dhcp_stage = MIP_DHCP_DISCOVER;
                    break;
                }
                ret = los_mip_dhcp_process_ack(msgs->p, dev);
                netbuf_free(msgs->p);
                msgs->p = NULL;
                los_mip_delete_up_sktmsg(msgs);
                if (MIP_OK != ret)
                {
                    dhcp_retry++;
                    dhcp_stage = MIP_DHCP_DISCOVER;
                    break;
                }
                dhcp_stage = MIP_DHCP_ARP_REQUEST;
                tmp_ipaddr.addr = g_requested_ip;
                break; 
            case MIP_DHCP_ARP_REQUEST:
                los_mip_arp_request(dev, &tmp_ipaddr);
                los_mip_delay(300);/* wait 300 ms to find if there are any device seted this ip */
                if (MIP_OK == los_mip_find_arp_entry(&tmp_ipaddr, NULL)) 
                {
                    g_requested_ip = 0x0;
                    g_requested_netmask = 0x0;
                    g_requested_gw = 0x0;
                    dhcp_stage = MIP_DHCP_DECLINE;
                    if (NULL != g_dhcp_callback)
                    {
                        g_dhcp_callback(MIP_DHCP_ERR_ADDR_CONFLICT, MIP_DHCP_STATE_PROCESSING);
                    }
                    break;
                }
                dev->ip_addr.addr = g_requested_ip;
                dev->netmask.addr = g_requested_netmask;
                dev->gw.addr = g_requested_gw;
                if (NULL != g_dhcp_callback)
                {
                    g_dhcp_callback(MIP_DHCP_ERR_NONE, MIP_DHCP_STATE_FINISHED);
                }
                break;
            case MIP_DHCP_DECLINE:
                /* todo: tell dhcp server this ip is already be used */
                buf = los_mip_dhcp_decline(dev);
                ret = los_mip_snd_sktmsg(con, buf); 
                if (ret != MIP_OK)
                {
                    dhcp_retry++;
                    netbuf_free(buf);
                    break;
                }
                dhcp_stage = MIP_DHCP_DISCOVER;
                break;
            default:
                break;
        }
        if (dev->ip_addr.addr != MIP_IP_ANY)
        {
            /* dhcp ok, so exit the task */
            break;
        }
    }
    los_mip_delete_conn(con);
    con = NULL;
    dev->autoipflag = AUTO_IP_DONE;
    /* destroy the dhcp task */
    los_mip_thread_delete(g_dhcp_task_id);
    g_dhcp_task_id = NULL;
}

/*****************************************************************************
 Function    : los_mip_dhcp_start
 Description : create 
 Input       : ethif @ device's pointer that witch device will do dhcp.
               func @ callback function, to tell up layer dhcp finished 
 Output      : None
 Return      : MIP_OK process done.
 *****************************************************************************/
int los_mip_dhcp_start(struct netif *ethif, dhcp_callback func)
{
    g_dhcp_callback = func;
    g_dhcp_task_id = los_mip_thread_new("DHCP_TASK", 
            los_mip_dhcp_task , 
            (void *)ethif, 
            MIP_DHCP_TASK_SIZE, 
            MIP_DHCP_TASK_PRIO);
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_wait_finish
 Description : wait dhcp function finished. 
 Input       : ethif @ dev's pointer that witch device is waitting dhcp done.
 Output      : None
 Return      : MIP_OK process done.
 *****************************************************************************/
int los_mip_dhcp_wait_finish(struct netif *ethif)
{
    if (NULL == ethif)
    {
        return MIP_OK;
    }
    while (ethif->autoipflag != AUTO_IP_DONE)
    {
        los_mip_delay(20);
    }
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_dhcp_stop
 Description : stop dhcp task. 
 Input       : ethif @ dev's pointer that witch device is waitting dhcp done.
 Output      : None
 Return      : MIP_OK process done.
 *****************************************************************************/
int los_mip_dhcp_stop(void)
{
    los_mip_thread_delete(g_dhcp_task_id);
    return MIP_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
