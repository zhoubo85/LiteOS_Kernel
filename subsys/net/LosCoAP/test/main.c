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
#include <stdio.h>
#include <string.h>

//#include "../include/los_unix.h"
#include "../include/coap_core.h"
#include "../include/los_coap.h"


int handle_coap_response(struct _coap_context_t *ctx, coap_msg_t *msg)
{
	printf("coap msgs: %s \n", msg->payload);
	return 0;	
}

int main()
{
	void *remoteser = NULL;
	coap_context_t *ctx = NULL;
	coap_option_t *opts = NULL;
	unsigned char blockdata = 0x2;
	coap_msg_t *msg = NULL;
	int ret  = 0;
	printf("1 \n");
	remoteser = los_coap_new_resource("192.168.206.41", 5683);
	printf("2 \n");
	ctx = los_coap_malloc_context(remoteser);
	printf("3 \n");
	opts = los_coap_add_option_to_list(opts, 
								COAP_OPTION_URI_PATH, 
								".well-known", 11);
	printf("4 \n");
	opts = los_coap_add_option_to_list(opts, 
								COAP_OPTION_URI_PATH, 
								"core", 4);
printf("5 \n");
	opts = los_coap_add_option_to_list(opts, 
								COAP_OPTION_BLOCK2, 
								&blockdata, 1);
printf("6 \n");
	msg = los_coap_new_msg(ctx,
							COAP_MESSAGE_CON, 
							COAP_REQUEST_GET, 
							opts, 
							NULL, 
							0);	
printf("7 \n");
	ret = los_coap_register_handler(ctx, handle_coap_response);
	if (ret < 0)
	{
		printf("register func err\n");
	}	
	ret = los_coap_send(ctx, msg);
	if (ret < 0)
	{
		printf("send err\n");
	}
printf("8 \n");
	ret = los_coap_read(ctx);
	if (ret < 0)
	{
		printf("recv err\n");
	}
	printf("9 \n");
	return 0;	
}