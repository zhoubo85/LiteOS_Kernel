
#include <stdlib.h>
#include <string.h>

#include "los_mip_err.h"
#include "los_mip_mem.h"
#include "los_mip_config.h"

/* include LiteOS header files */
#include "los_config.h"
#include "los_memory.h"
#include "los_api_dynamic_mem.h"


#if defined ( __CC_ARM ) /* MDK Compiler */
__align(4) u8_t g_pool_pkgbuf[LOS_MPOOL_PKGBUF_SIZE] = {0};
__align(4) u8_t g_pool_msgs[LOS_MPOOL_MSGS_SIZE] = {0};
__align(4) u8_t g_pool_sockets[LOS_MPOOL_SOCKET_SIZE] = {0};
#endif

u8_t *g_mpools[MPOOL_MAX] = {NULL};


int los_mpools_init(void)
{
	UINT32 uwRet;
	
	g_mpools[MPOOL_NETBUF] = g_pool_pkgbuf;
	uwRet = LOS_MemInit(g_mpools[MPOOL_NETBUF], LOS_MPOOL_PKGBUF_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }
    
	g_mpools[MPOOL_MSGS] = g_pool_msgs;
	uwRet = LOS_MemInit(g_mpools[MPOOL_MSGS], LOS_MPOOL_MSGS_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }  
	g_mpools[MPOOL_SOCKET] = g_pool_sockets;
	uwRet = LOS_MemInit(g_mpools[MPOOL_SOCKET], LOS_MPOOL_SOCKET_SIZE);
    if (LOS_OK != uwRet) 
    {
    	return -MIP_POOL_INIT_FAILED;
    }     
    return MIP_OK;
}

void *los_mpools_malloc(mpool_t type, u32_t size)
{
    void *p = NULL;
    if (type >= MPOOL_MAX)
    {
        return NULL;
    }
    p = LOS_MemAlloc(g_mpools[type], size);
    
	return p;
}

int los_mpools_free(mpool_t type, void *mem)
{
    UINT32 uwRet;
    if (type >= MPOOL_MAX || NULL == mem)
    {
        return -MIP_ERR_PARAM;
    }
    uwRet = LOS_MemFree(g_mpools[type], mem);
    if (LOS_OK != uwRet) 
    {
        return -MIP_MEM_FREE_FAILED;
    }
	return MIP_OK;
}


