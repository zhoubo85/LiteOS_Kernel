#include "los_sys.h"
#include "los_tick.h"
#include "los_task.ph"
#include "los_config.h"
#include "cmsis_os.h"

#include "../include/coap_core.h"
#include "../include/los_coap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int handle_coap_response(struct _coap_context_t *ctx, coap_msg_t *msg)
{
	//printf("coap msgs: %s \n", msg->payload);
	return 0;	
}

static void coaptest_task(void *arg)
{
	void *remoteser = NULL;
	coap_context_t *ctx = NULL;
	coap_option_t *opts = NULL;
	unsigned char blockdata = 0x2;
	coap_msg_t *msg = NULL;
	int ret  = 0;
    
    los_coap_stack_init();
    
	remoteser = los_coap_new_resource("192.168.206.41", 5683);

	ctx = los_coap_malloc_context(remoteser);

	opts = los_coap_add_option_to_list(opts, 
								COAP_OPTION_URI_PATH, 
								".well-known", 11);

	opts = los_coap_add_option_to_list(opts, 
								COAP_OPTION_URI_PATH, 
								"core", 4);

	opts = los_coap_add_option_to_list(opts, 
								COAP_OPTION_BLOCK2, 
								(char *)&blockdata, 1);

	msg = los_coap_new_msg(ctx,
							COAP_MESSAGE_CON, 
							COAP_REQUEST_GET, 
							opts, 
							NULL, 
							0);	

	ret = los_coap_register_handler(ctx, handle_coap_response);
	if (ret < 0)
	{
		//printf("register func err\n");
        while(1);
	}	
	ret = los_coap_send(ctx, msg);
	if (ret < 0)
	{
		//printf("send err\n");
         while(1);
	}

	ret = los_coap_read(ctx);
	if (ret < 0)
	{
		//printf("recv err\n");
         while(1);
	}

}




/*!
    \brief      initialize the udp_echo application
    \param[in]  none
    \param[out] none
    \retval     none
*/
void coap_test_init(void)
{
    osThreadDef_t thread_def;

    thread_def.name = "testcoap";
    thread_def.stacksize = 1024;//DEFAULT_THREAD_STACKSIZE;
    thread_def.tpriority = osPriorityLow;
    thread_def.pthread = (os_pthread)coaptest_task;
    osThreadCreate(&thread_def, NULL);
}
