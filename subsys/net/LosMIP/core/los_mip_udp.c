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


/*****************************************************************************
 Function    : los_mip_udp_input
 Description : parse udp input package and send payload to upper layer
 Input       : p @ package's pointer
               dev @ net dev's handle which the data come from
               src @ source ip address of the package
               dst @ dest ip address of the package.
 Output      : None
 Return      : MIP_OK process ok, other value mean some error happened.
 *****************************************************************************/
int los_mip_udp_input(struct netbuf *p,
                      struct netif *dev, 
                      ip_addr_t *src, 
                      ip_addr_t *dst)
{
    struct mip_conn *tmpcon = NULL;
    struct udp_hdr *udph;
    s8_t doneflag = 0;
    u32_t chksum = 0x0;
    u8_t discardflag = 1;
    int ret = MIP_OK;
    struct skt_rcvd_msg *upmsg = NULL; 
    
    if ((NULL == p) || (NULL == dev))
    {
        return -MIP_ERR_PARAM;
    }
    if (p->len < MIP_UDP_HDR_LEN)
    {
        netbuf_free(p);
        return -MIP_UDP_PKG_LEN_ERR;
    }
    udph = (struct udp_hdr *)p->payload;
    
    if (udph->chksum != 0)
    {
        /* calc the checksum */
        if ((u16_t)TYPE_IPV4 == los_mip_get_ip_ver(p)) 
        {
            chksum = los_mip_checksum_pseudo_ipv4(p, (u8_t)MIP_PRT_UDP, p->len, src, dst);
            if (chksum != 0)
            {
                netbuf_free(p);
                return -MIP_UDP_PKG_CHKSUM_ERR;
            }
            /* check ok now, send data to sockets */
            while (doneflag >= 0)
            {
                tmpcon = los_mip_walk_con(&doneflag);
                if (NULL == tmpcon)
                {
                    /* all connections are walk through */
                    break;
                }
                if ((tmpcon->type == CONN_UDP) 
                    && (udph->dest == tmpcon->ctlb.udp->lport))
                {
                    /* get udp for current connection, */
                    if ((MIP_IP_ANY == tmpcon->ctlb.udp->localip.addr)
                        || (dev->ip_addr.addr == tmpcon->ctlb.udp->localip.addr))
                    {
                        /* todo :make select msg to send to connection */
                        
                        /* make data msg to send to connection */
                        discardflag = 0; 
                        ret = los_mip_jump_header(p, MIP_UDP_HDR_LEN);
                        if (ret != MIP_OK)
                        {
                            discardflag = 1;
                            break;
                        }
                        upmsg = los_mip_new_up_sktmsg(p, src, udph->src);
                        if (NULL == upmsg)
                        {
                            discardflag = 1;
                            break;
                        }
                        ret = los_mip_snd_msg_to_con(tmpcon, upmsg);
                        if (ret != MIP_OK)
                        {
                            los_mip_delete_up_sktmsg(upmsg);
                            discardflag = 1;
                            upmsg = NULL;
                        }
                        break;
                    }
                }
            }
            if (discardflag)
            {
                /* package not for any connections, so discard it */
                netbuf_free(p);
                p = NULL;
            }
        }
    }
    
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mip_udp_output
 Description : send out a udp package to ip layer.
 Input       : udpctl @ udp connection's info
               p @ the udp package buf pointer
               dst_ip @ dest ip address udp data will send to
               dst_port @ dest port that udp data will send to.
 Output      : None
 Return      : ret @ 0 means send ok, other value means error happened.
 *****************************************************************************/
int los_mip_udp_output(struct udp_ctl *udpctl, 
                       struct netbuf *p, 
                       ip_addr_t *dst_ip, 
                       u16_t dst_port)
{
    int ret = MIP_OK;
    struct netif *dev = NULL;
    struct udp_hdr *udph;
    
    if ((NULL == p) || (NULL == udpctl))
    {
        return -MIP_ERR_PARAM;
    }
    
    if (sizeof(ip_addr_t) == sizeof(struct ipv4_addr))
    {
        /* find the dev to send out the data */
        dev = los_mip_ip_route(dst_ip);
        
        /* add udp header */
        los_mip_jump_header(p, -MIP_UDP_HDR_LEN);
        udph = (struct udp_hdr *)p->payload;
        udph->chksum = 0x00;
        udph->len = MIP_HTONS(p->len);
        udph->dest = dst_port;
        if (udpctl->lport == 0x00)
        {
            /* generate the local port */
            udpctl->lport = los_mip_generate_port();
            if (MIP_OK != los_mip_add_port_to_used_list(udpctl->lport))
            {
                return -MIP_CON_PORT_RES_USED_OUT;
            }
        }
        udph->src = udpctl->lport;
        udph->chksum = los_mip_checksum_pseudo_ipv4(p, (u8_t)MIP_PRT_UDP, 
                                                    p->len, &dev->ip_addr, dst_ip);
        
        
        ret = los_mip_ipv4_output(dev, p, &dev->ip_addr, dst_ip,
                                  udpctl->ttl, udpctl->tos, MIP_PRT_UDP);
    }
    return ret;
}
