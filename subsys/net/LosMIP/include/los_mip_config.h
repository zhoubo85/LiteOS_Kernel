#ifndef _LOS_MIP_CONFIG_H
#define _LOS_MIP_CONFIG_H

#define LOS_MPOOL_PKGBUF_SIZE   1024*4
#define LOS_MPOOL_MSGS_SIZE     512
#define LOS_MPOOL_SOCKET_SIZE   512

/* mip resource(sockets number, arp table size ...) max size */
#define MIP_RES_SIZE_MAX        6

/* Message queue max messages in queue */
#define MIP_MESG_QUEUE_LENGTH	MIP_RES_SIZE_MAX
/* arp wait queue size */
#define LOS_MIP_MAX_WAIT_CON    MIP_RES_SIZE_MAX
#define MIP_ARP_TAB_SIZE        MIP_RES_SIZE_MAX

#define MIP_CONN_MAX            MIP_RES_SIZE_MAX
#define MIP_CONN_RCVBOX_SIZE    MIP_RES_SIZE_MAX
#define MIP_CONN_ACPTBOX_SIZE   MIP_RES_SIZE_MAX

#define MIP_HW_MTU_VLAUE        1500

#endif
