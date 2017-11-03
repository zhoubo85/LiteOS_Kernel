#ifndef _LOS_MIP_ETHIF_H
#define _LOS_MIP_ETHIF_H

#include "los_mip_err.h"
#include "los_mip_netif.h"
#include "cmsis_os.h"


int los_mip_dev_eth_init(struct netif *netif);

#endif
