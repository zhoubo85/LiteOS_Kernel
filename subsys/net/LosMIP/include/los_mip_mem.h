#ifndef _LOS_MIP_MEM_H
#define _LOS_MIP_MEM_H

#include "los_mip_typed.h"

typedef enum {
	MPOOL_NETBUF,
    MPOOL_MSGS,
    MPOOL_SOCKET,
	MPOOL_MAX
}mpool_t;


int los_mpools_init(void);
void *los_mpools_malloc(mpool_t type, u32_t size);
int los_mpools_free(mpool_t type, void *mem);


#endif
