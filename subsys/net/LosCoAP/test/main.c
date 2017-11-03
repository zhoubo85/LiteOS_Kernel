//#include "../include/los_unix.h"
#include "../include/coap_core.h"
#include "../include/los_coap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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