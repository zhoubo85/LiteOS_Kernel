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
 



#define WITH_LITEOS

#ifdef WITH_LITEOS
#include "los_config.h"
#include "los_memory.h"
#include "los_api_dynamic_mem.h"

#include "los_mip.h"

#include "los_coap.h"
#include "coap_core.h"
#include "los_coap_err.h"


#define RANDOM_TOKEN_LEN 8
/* max struct size that can malloc from mempool */
#define G_LOS_COAP_SMALL_STRUCT_MAX 64
/* mempool size of coap */
#define G_LOS_COAP_MEM_POOL_SIZE 2048
unsigned char g_los_coap_mempool[G_LOS_COAP_MEM_POOL_SIZE];
unsigned char g_los_token[4] = {0};
unsigned char *g_coap_mem = NULL;

int los_coap_mip_send(void *handle, char *buf, int size);
int los_coap_mip_read(void *handle, char *buf, int size);

/* coap recive and send function callback */
struct udp_ops network_ops = 
{
    .network_read = los_coap_mip_read,
    .network_send = los_coap_mip_send
};

/*****************************************************************************
 Function    : los_coap_get_random
 Description : generate a ramdon data 
 Input       : None
 Output      : None
 Return      : ret @ 32bit random data.
 *****************************************************************************/
static unsigned int los_coap_get_random(void)
{
	unsigned int ret;
	LOS_TickCountGet();
	srand((unsigned)LOS_TickCountGet());
	ret = rand() % RAND_MAX;
	return ret;
}

/*****************************************************************************
 Function    : los_coap_generate_token
 Description : generate a ramdon token for coap message. 
 Input       : None
 Output      : token @ token buf that used to store token
 Return      : RANDOM_TOKEN_LEN @ token length.
 *****************************************************************************/
int los_coap_generate_token(unsigned char *token)
{
	unsigned int ret;
	srand((unsigned)LOS_TickCountGet());
	ret = rand() % RAND_MAX;
    token[0] = (unsigned char)ret;
    token[1] = (unsigned char)(ret>>8);
    token[2] = (unsigned char)(ret>>16);
    token[3] = (unsigned char)(ret>>24);
	return RANDOM_TOKEN_LEN;
}

/*****************************************************************************
 Function    : los_coap_stack_init
 Description : init coap stack, such as mempool 
 Input       : None
 Output      : None
 Return      : LOS_COAP_OK means init ok, other means failed.
 *****************************************************************************/
int los_coap_stack_init(void)
{
	UINT32 uwRet;
	g_coap_mem = g_los_coap_mempool;
	uwRet = LOS_MemInit(g_coap_mem, G_LOS_COAP_MEM_POOL_SIZE);
    if (LOS_OK != uwRet) 
    {
        return -LOS_COAP_STATACK_INIT_FAILED;
    }
    
 	return LOS_COAP_OK;   
}

/*****************************************************************************
 Function    : los_coap_malloc
 Description : malloc memory from mempool 
 Input       : size @ the length that want to malloc from mempool.
 Output      : None
 Return      : tmp @ malloced buf's pointer.
 *****************************************************************************/
void *los_coap_malloc(int size)
{
    char *tmp = NULL;
    if (size <= 0)
    {
        return NULL;
    }
    tmp = (char *)LOS_MemAlloc(g_coap_mem, size);
    return (void *)tmp;
}

/*****************************************************************************
 Function    : los_coap_free
 Description : free memory from mempool 
 Input       : p @ memory that want to free.
 Output      : None
 Return      : LOS_COAP_OK free ok, other value means failed.
 *****************************************************************************/
int los_coap_free(void *p)
{
    if (NULL == p)
	{
		return -LOS_COAP_PARAM_NULL;
	}

    LOS_MemFree(g_coap_mem, p);
    return LOS_COAP_OK;
}

/*****************************************************************************
 Function    : los_coap_check_validip
 Description : check if the ip address if valid ip address 
 Input       : ipaddr @ string format ip address that need to check.
 Output      : None
 Return      : LOS_COAP_OK means valid, other means invalid.
 *****************************************************************************/
static int los_coap_check_validip(char *ipaddr)
{
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int ret = 0;
    if(NULL == ipaddr)
    {
        return -LOS_COAP_PARAM_NULL;
    }
    ret = sscanf(ipaddr, "%d.%d.%d.%d", &a,&b,&c,&d);
    if (ret != 4)
    {
        return -LOS_COAP_SOCKET_ADDRCHK_ERR;
    }
    if ((a >= 0 && a <= 255)
        && (b >=0 && b <= 255)
        && (c >=0 && c <= 255)
        && (d >=0 && d <= 255))
    {
        return LOS_COAP_OK;
    }
    return -LOS_COAP_SOCKET_ADDRCHK_ERR;
}

/*****************************************************************************
 Function    : los_coap_new_resource
 Description : create socket resource for coap connection
 Input       : ipaddr @ remote endpoint or server's ipaddress
               port @ remote  endpoint or server's port number
 Output      : None
 Return      : tmp @ socket resource's pointer
 *****************************************************************************/
void *los_coap_new_resource(char *ipaddr, unsigned short port)
{
    int ret = 0;
    struct udp_res_t *tmp = NULL;
    struct sockaddr_in localaddr;
    if (NULL == ipaddr)
    {
        return NULL;
    }
    ret = los_coap_check_validip(ipaddr);
    if (ret != LOS_COAP_OK)
    {
        return NULL;
    }
    tmp = (struct udp_res_t *)LOS_MemAlloc(g_coap_mem, sizeof(struct udp_res_t));
    if (NULL == tmp)
    {
        return NULL;
    }
    memset(tmp, 0, sizeof(struct udp_res_t));
    tmp->fd = los_mip_socket(AF_INET, SOCK_DGRAM, 0);
    if (tmp->fd < 0)
    {
        LOS_MemFree(g_coap_mem, tmp);
        return NULL;
    }
    
	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localaddr.sin_port = htons(5000);
	los_mip_bind(tmp->fd, (struct sockaddr *)&localaddr, sizeof(localaddr));
    
    tmp->remoteAddr.sin_addr.s_addr = inet_addr((const char *)ipaddr);
    tmp->remoteAddr.sin_port = htons(port);
    tmp->remoteAddr.sin_family = AF_INET;
    return (void *)tmp;
}

/*****************************************************************************
 Function    : los_coap_delete_resource
 Description : release upd resource
 Input       : res @ udp resource pointer
 Output      : None
 Return      : LOS_COAP_OK means free ok. other means free failed.
 *****************************************************************************/
int los_coap_delete_resource(void *res)
{
    struct udp_res_t *tmp = NULL;
    if (NULL == res)
	{
		return -LOS_COAP_PARAM_NULL;
	}
    tmp = (struct udp_res_t *)res;
    if (tmp->fd >= 0)
    {
        los_mip_close(tmp->fd);
        tmp->fd = -1;
    }
    LOS_MemFree(g_coap_mem, tmp);
    return LOS_COAP_OK;
}

/*****************************************************************************
 Function    : los_coap_malloc_context
 Description : malloc a coap connection and init it.
 Input       : res @ udp resource's pointer.
 Output      : None
 Return      : tmp @ coap connection's pointer.
 *****************************************************************************/
coap_context_t *los_coap_malloc_context(void *res)
{
	coap_context_t *tmp = NULL;
	if (NULL == res)
	{
		return NULL;
	}
	
	tmp = (coap_context_t *)LOS_MemAlloc(g_coap_mem, sizeof(coap_context_t));
	if (NULL == tmp)
	{
		return NULL;
	}
    memset(tmp, 0, sizeof(coap_context_t));
	
	/* FIXED ME */
	tmp->udpio = res;
	
	tmp->sndbuf.buf = (unsigned char *)LOS_MemAlloc(g_coap_mem, LOSCOAP_CONSTRAINED_BUF_SIZE);
	if (NULL == tmp->sndbuf.buf)
	{
        LOS_MemFree(g_coap_mem, tmp);
		return NULL;
	}
	tmp->sndbuf.len = LOSCOAP_CONSTRAINED_BUF_SIZE;
	
	tmp->rcvbuf.buf = (unsigned char *)LOS_MemAlloc(g_coap_mem, LOSCOAP_CONSTRAINED_BUF_SIZE);
	if (NULL == tmp->rcvbuf.buf)
	{
        LOS_MemFree(g_coap_mem, tmp);
        LOS_MemFree(g_coap_mem, tmp->sndbuf.buf);
		return NULL;
	}
    
	tmp->rcvbuf.len = LOSCOAP_CONSTRAINED_BUF_SIZE;
	tmp->msgid = (unsigned short)los_coap_get_random();
    tmp->netops = &network_ops;
    
    tmp->response_handler = NULL;
	return tmp;	
}

/*****************************************************************************
 Function    : los_coap_free_context
 Description : free a coap connection's resource and memory.
 Input       : ctx @ coap connection's pointer.
 Output      : None
 Return      : LOS_COAP_OK @ means free ok. other means error happend.
 *****************************************************************************/
int los_coap_free_context(coap_context_t *ctx)
{
	coap_context_t *tmp = ctx;
    send_queue_t *tmpque = NULL;
    
	if (NULL == ctx)
	{
		return -LOS_COAP_PARAM_NULL;
	}
	if (NULL != tmp->udpio)
    {
        los_coap_delete_resource(tmp->udpio);
        tmp->udpio = NULL;
    }
    while(NULL != ctx->resndque)
    {
        tmpque = ctx->resndque;
        if (NULL != ctx->resndque->msg)
        {
            los_coap_delete_msg(ctx->resndque->msg);
            ctx->resndque->msg = NULL;
        }
        ctx->resndque = ctx->resndque->next;
        LOS_MemFree(g_coap_mem, tmpque);
        tmpque = NULL;
    }
    while(NULL != ctx->sndque)
    {
        tmpque = ctx->sndque;
        if (NULL != ctx->sndque->msg)
        {
            los_coap_delete_msg(ctx->sndque->msg);
            ctx->sndque->msg = NULL;
        }
        ctx->sndque = ctx->sndque->next;
        LOS_MemFree(g_coap_mem, tmpque);
        tmpque = NULL;
    }
    if (NULL != tmp->sndbuf.buf)
    {
        LOS_MemFree(g_coap_mem, tmp->sndbuf.buf);
        tmp->sndbuf.buf = NULL;
        tmp->sndbuf.len = 0;
    }
    if (NULL != tmp->rcvbuf.buf)
    {
        LOS_MemFree(g_coap_mem, tmp->rcvbuf.buf);
        tmp->rcvbuf.buf = NULL;
        tmp->rcvbuf.len = 0;
    }
    tmp->netops = NULL;
    tmp->response_handler = NULL;
    LOS_MemFree(g_coap_mem, ctx);
    
	return LOS_COAP_OK;	
}

/*****************************************************************************
 Function    : los_coap_mip_send
 Description : coap send function.
 Input       : handle @ udp resource pointer
               buf @ coap byte stream data
               size @ coap byte stream data's length.
 Output      : None
 Return      : n @ size that send out by tcp/ip stack.
 *****************************************************************************/
int los_coap_mip_send(void *handle, char *buf, int size)
{
    int n = 0;
    struct udp_res_t *res = NULL;
    if (NULL == handle || NULL == buf)
    {
        return -LOS_COAP_PARAM_NULL;
    }
    res = (struct udp_res_t *)handle;
    if (res->fd < 0)
    {
        return -LOS_COAP_SOCKET_HANDLER_ERR;
    }
	n = los_mip_sendto(res->fd,
			buf,
			size,
			0, 
			(struct sockaddr *)&res->remoteAddr,
			sizeof(struct sockaddr_in));
    return n;
}

/*****************************************************************************
 Function    : los_coap_mip_read
 Description : coap read function.
 Input       : handle @ udp resource pointer
               buf @ used for storing coap byte stream data
               size @ max size that want to read out.
 Output      : None
 Return      : n @ size that read from tcp/ip stack.
 *****************************************************************************/
int los_coap_mip_read(void *handle, char *buf, int size)
{
    struct timeval tv;
    int n = 0;
    struct sockaddr_in fromAddr;
    socklen_t fromLen;
    struct udp_res_t *res = NULL;
    if (NULL == handle || NULL == buf)
    {
        return -LOS_COAP_PARAM_NULL;
    }
    res = (struct udp_res_t *)handle;
    if (res->fd < 0)
    {
        return -LOS_COAP_SOCKET_HANDLER_ERR;
    }
    
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if(los_mip_setsockopt(res->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        return -LOS_COAP_SOCKET_SETOPT_ERR;
    }
    
    memset(&fromAddr,0, sizeof(fromAddr));  
    fromAddr.sin_family = AF_INET;  
    fromAddr.sin_addr.s_addr = htonl(INADDR_ANY);  
    fromAddr.sin_port = res->remoteAddr.sin_port;  
    fromLen=sizeof(fromAddr);
    
    n = los_mip_recvfrom( res->fd, 
                buf, size, 
                0, 
                (struct sockaddr *)&fromAddr, 
                &fromLen );
    return n;
}

/*****************************************************************************
 Function    : los_coap_delay
 Description : delay function , use this task will sleep xxx ms
 Input       : ms @ the time that need sleep
 Output      : None
 Return      : LOS_COAP_OK @ sleep ok.
 *****************************************************************************/
int los_coap_delay(unsigned int ms)
{
    (void)LOS_TaskDelay(ms);
    return LOS_COAP_OK;
}

#endif /* WITH_LITEOS */

