#ifndef _LOS_COAP_H
#define _LOS_COAP_H

#include "coap_core.h"

int los_coap_stack_init(void);

void *los_coap_malloc(int size);
int los_coap_free(void *p);
int los_coap_delay(unsigned int ms);
/**
 * use server ip and port to create coap_context_t resources
 */
void *los_coap_new_resource(char *ipaddr, unsigned short port);

int los_coap_delete_resource(void *res);
/**
 * Create a new coap_context_t object that will hold the CoAP resources.
 */
coap_context_t *los_coap_malloc_context(void *res);
int los_coap_free_context(coap_context_t *ctx);

coap_option_t * los_coap_add_option_to_list(coap_option_t *head, 
								unsigned short option, 
								char *value, int len);
int los_coap_free_option(coap_option_t *head);
int los_coap_add_option(coap_msg_t *msg, coap_option_t *opts);
int los_coap_generate_token(unsigned char *token);
int los_coap_add_token(coap_msg_t *msg, char *tok, int tklen);
int los_coap_add_paylaod(coap_msg_t *msg, char *payload, int len);
coap_msg_t *los_coap_new_msg(coap_context_t *ctx,
								unsigned char msgtype, 
								unsigned char code, 
								coap_option_t *optlist, 
								unsigned char *paylaod, 
								int payloadlen);
int los_coap_delete_msg(coap_msg_t *msg);

int los_coap_register_handler(coap_context_t *ctx, msghandler func);
int los_coap_add_resource(coap_context_t *ctx, coap_res_t *res);
int los_coap_read(coap_context_t *ctx);
int los_coap_send(coap_context_t *ctx, coap_msg_t *msg);

int los_coap_addto_sndqueue(coap_context_t *ctx, coap_msg_t *msg);
int los_coap_remove_sndqueue(coap_context_t *ctx, coap_msg_t *msg);
int los_coap_discard_resndqueue(coap_context_t *ctx);
#endif
