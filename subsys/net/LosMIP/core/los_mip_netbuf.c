#include <stdlib.h>
#include <string.h>
#include "los_mip_config.h"
#include "los_mip_err.h"
#include "los_mip_typed.h"
#include "los_mip_mem.h"
#include "los_mip_netbuf.h"

static u32_t g_wait_arp_array[LOS_MIP_MAX_WAIT_CON] = {0};

int los_mip_jump_header(struct netbuf *p, s16_t size)
{
    if (NULL == p)
    {
        return MIP_OK;
    }
    if (p->len <= size)
    {
        return -MIP_MPOOL_JUMP_LEN_ERR;
    }
    if (((u8_t *)p->payload + size) < ((u8_t *)p + sizeof(struct netbuf)))
    {
         return -MIP_MPOOL_JUMP_LEN_ERR;
    }
    p->len -= size;
    p->payload = (u8_t *)p->payload + size;
    return MIP_OK;
}

struct netbuf *netbuf_alloc(nebubuf_layer layer, u16_t length)
{
    struct netbuf *p = NULL;
    int offset = 0;
    switch(layer)
    {
        case NETBUF_TRANS:
            offset = NETBUF_RESERVE_LEN + NETBUF_LINK_HLEN + NETBUF_IP_HLEN + NETBUF_TRANS_HLEN;
            break;
        case NETBUF_IP:
            offset = NETBUF_RESERVE_LEN + NETBUF_LINK_HLEN + NETBUF_IP_HLEN;
            break;
        case NETBUF_RAW:
            offset = 0;
            break; 
        default:
            return NULL;
    }
    p = (struct netbuf *)los_mpools_malloc(MPOOL_NETBUF, offset + length + sizeof(struct netbuf));
    if(NULL != p)
    {
        memset(p, 0, offset + length + sizeof(struct netbuf));
        p->len = length;
        p->next = NULL;
        p->payload = (u8_t *)p + sizeof(struct netbuf) + offset;
        p->ref = 1;
    }
    return p;
}

int netbuf_free(struct netbuf *p)
{
    int ret = 0;
    
    if (NULL == p)
        return MIP_OK;
    ret = los_mpools_free(MPOOL_NETBUF, p);
    return ret;
}

int netbuf_que_for_arp(struct netbuf *p)
{
    int i = 0;
    int nqued = 0;
    int pos = -1;

    if (NULL == p)
    {
        return MIP_OK;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i++)
    {
        if (g_wait_arp_array[i] == (u32_t)(p))
        {
            nqued = 1;
            return MIP_OK;
        }
        if (g_wait_arp_array[i] == 0x0 && pos == -1)
        {
            pos = i;
        }
    }
    
    if (!nqued && pos != -1)
    {
        g_wait_arp_array[pos] = (u32_t)p;
    }
    else
    {
        return -MIP_ARP_QUE_FULL;
    }
    return MIP_OK;
}

int netbuf_get_if_queued(struct netbuf *p)
{
    int i = 0;
    if (NULL == p)
    {
        return MIP_FALSE;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i++)
    {
        if (g_wait_arp_array[i] == (u32_t)(p))
        {
            return MIP_TRUE;
        }
    }
    return MIP_FALSE;
}

int netbuf_remove_queued(struct netbuf *p)
{
    int i = 0;
    if (NULL == p)
    {
        return MIP_OK;
    }
    for (i = 0; i < LOS_MIP_MAX_WAIT_CON; i++)
    {
        if (g_wait_arp_array[i] == (u32_t)(p))
        {
            g_wait_arp_array[i] = 0x0;
            return MIP_TRUE;
        }
    }
    return MIP_OK;
}
