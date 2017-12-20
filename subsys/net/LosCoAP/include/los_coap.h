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
 
#ifndef _LOS_COAP_H
#define _LOS_COAP_H

#include "coap_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

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

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_COAP_H */
