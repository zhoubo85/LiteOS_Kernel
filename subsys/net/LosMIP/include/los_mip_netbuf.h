#ifndef _LOS_MIP_NETPBUF_H
#define _LOS_MIP_NETPBUF_H

#include "los_mip_typed.h"

/* network packet buffer struct */
struct netbuf 
{
    /* next pbuf in singly linked pbuf chain */
    struct netbuf *next;

    /* pointer to the actual data in the buffer */
    void *payload;

    /* length of this buffer */
    u16_t len;

    /* misc flags */
    u8_t flags;
    u16_t ipver;/* ip package version */
    /*
        the reference count always equals the number of pointers
        that refer to this pbuf. This can be pointers from an application,
        the stack itself, or pbuf->next pointers from a chain.
   */
    u16_t ref;
};

typedef enum {
    NETBUF_TRANS,   /* transport layer buf(tcp,udp ... ) */
    NETBUF_IP,      /* ip layer buf */
    NETBUF_LINK,    /* link layer buf(add ethernet header ...) */
    NETBUF_RAW
} nebubuf_layer;

#define NETBUF_TRANS_HLEN 20
#define NETBUF_IP_HLEN 20
#define NETBUF_LINK_HLEN 14
#define NETBUF_RESERVE_LEN 32


int los_mip_jump_header(struct netbuf *p, s16_t size);
struct netbuf *netbuf_alloc(nebubuf_layer layer, u16_t length);
int netbuf_free(struct netbuf *p);
int netbuf_que_for_arp(struct netbuf *p);
int netbuf_get_if_queued(struct netbuf *p);
int netbuf_remove_queued(struct netbuf *p);

#endif
