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

/* include LiteOS header files */
#include "los_config.h"
#include "los_memory.h"
#include "los_api_dynamic_mem.h"

#include "los_mip_err.h"
#include "los_mip_mem.h"
#include "los_mip_config.h"

#if defined ( __CC_ARM ) /* MDK Compiler */
__align(4) u8_t g_pool_pkgbuf[LOS_MPOOL_PKGBUF_SIZE] = {0};
__align(4) u8_t g_pool_msgs[LOS_MPOOL_MSGS_SIZE] = {0};
__align(4) u8_t g_pool_sockets[LOS_MPOOL_SOCKET_SIZE] = {0};
__align(4) u8_t g_pool_tcp[LOS_MPOOL_TCP_SIZE] = {0};

#elif defined ( __ICCARM__ )/* IAR Compiler */
#pragma data_alignment=4
u8_t g_pool_pkgbuf[LOS_MPOOL_PKGBUF_SIZE] = {0};
u8_t g_pool_msgs[LOS_MPOOL_MSGS_SIZE] = {0};
u8_t g_pool_sockets[LOS_MPOOL_SOCKET_SIZE] = {0};
u8_t g_pool_tcp[LOS_MPOOL_TCP_SIZE] = {0};

#elif defined ( __GNUC__ )/* GNU Compiler */
u8_t g_pool_pkgbuf[LOS_MPOOL_PKGBUF_SIZE] = {0};
u8_t g_pool_msgs[LOS_MPOOL_MSGS_SIZE] = {0};
u8_t g_pool_sockets[LOS_MPOOL_SOCKET_SIZE] = {0};
u8_t g_pool_tcp[LOS_MPOOL_TCP_SIZE] = {0};

#endif

u8_t* g_mpools_p[MPOOL_MAX] = {NULL};

/*****************************************************************************
 Function    : los_mpools_init
 Description : call liteos function to init mip mempools
 Input       : None
 Output      : None
 Return      : MIP_OK means init success, other value means error happend.
 *****************************************************************************/
int los_mpools_init(void)
{
	UINT32 uwRet;
	
	g_mpools_p[MPOOL_NETBUF] = g_pool_pkgbuf;
	uwRet = LOS_MemInit(g_mpools_p[MPOOL_NETBUF], LOS_MPOOL_PKGBUF_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }
    
	g_mpools_p[MPOOL_MSGS] = g_pool_msgs;
	uwRet = LOS_MemInit(g_mpools_p[MPOOL_MSGS], LOS_MPOOL_MSGS_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }  
	g_mpools_p[MPOOL_SOCKET] = g_pool_sockets;
	uwRet = LOS_MemInit(g_mpools_p[MPOOL_SOCKET], LOS_MPOOL_SOCKET_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }
    
	g_mpools_p[MPOOL_TCP] = g_pool_tcp;
	uwRet = LOS_MemInit(g_mpools_p[MPOOL_TCP], LOS_MPOOL_TCP_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }      
    return MIP_OK;
}

/*****************************************************************************
 Function    : los_mpools_malloc
 Description : malloc memory from a mempool 
 Input       : type @ mempool type indicate which pool to malloc mem
               szie @ the size should malloc from mempool
 Output      : None
 Return      : p @ buf's pointer, if failed return NULL.
 *****************************************************************************/
void *los_mpools_malloc(mpool_t type, u32_t size)
{
    void *p = NULL;
    
    if (type >= MPOOL_MAX)
    {
        return NULL;
    }
    p = LOS_MemAlloc(g_mpools_p[type], size);
    
	return p;
}

/*****************************************************************************
 Function    : los_mpools_free
 Description : free memory from a mempool 
 Input       : type @ mempool type indicate which pool to free mem
               mem @ buf's address that need to free.
 Output      : None
 Return      : MIP_OK means free ok, other value means error happend.
 *****************************************************************************/
int los_mpools_free(mpool_t type, void *mem)
{
    UINT32 uwRet;
    
    if ((type >= MPOOL_MAX) || (NULL == mem))
    {
        return -MIP_ERR_PARAM;
    }
    uwRet = LOS_MemFree(g_mpools_p[type], mem);
    if (LOS_OK != uwRet) 
    {
        return -MIP_MEM_FREE_FAILED;
    }
	return MIP_OK;
}
