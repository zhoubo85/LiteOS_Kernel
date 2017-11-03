#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "los_mip.h"
#include "coap_core.h"
#include "los_coap.h"
#include "los_coap_err.h"
#include "los_oclink.h"

#define OCLINK_MAX_SEND_RETRY 500


static unsigned char   oclink_res[3]={'t'};
static unsigned char   oclink_res1[3]={'r'};
static unsigned char   oclink_res2[16]="ep=";

static cmdcallback los_cmd_func = NULL;
static char g_dev_sn[OCLINK_MAX_SN_LEN] = {0};
static char g_dev_sn_len = 0;
static char g_tok[COAP_TOKEN_LEN_MAX] = {0};
static char g_tok_len = 0;
static unsigned short g_iot_port = 0;
static char g_iot_server[OCLINK_MAX_ADDR_LEN] = {0};
static int oclink_state = OCLINK_STAT_INIT;
static int g_bind_finsh = 0;
static coap_context_t *ctx = NULL;
static int oclink_err_code = 0;
static char oclink_stop_flag = 0;
static UINT32 g_oclink_taskid = 0;

static int los_oclink_set_errstate(int *state);

static int los_oclink_set_errcode(int err)
{
    oclink_err_code = err;
    return 0;
}
int los_oclink_get_errcode(void)
{
    return oclink_err_code;
}


int los_oclink_set_param(oclink_param_t kind, void *data, int len)
{
    if(NULL == data)
    {
        return -1;
    }
    switch(kind)
    {
        case OCLINK_IOTS_ADDR:
            if (len >= OCLINK_MAX_ADDR_LEN)
            {
                return -1;
            }
            memcpy(g_iot_server, (char *)data, len);
            break;
        case OCLINK_IOTS_PORT:
            g_iot_port = *(unsigned short *)data;
            break;
        case OCLINK_IOTS_ADDRPORT:
            if (len > 21)
            {
                return -1;
            }
            sscanf((char *)data, "%s,%hu", g_iot_server, &g_iot_port);
            break;
        case OCLINK_IOT_DEVSN:
            if (len > OCLINK_MAX_SN_LEN)
            {
                g_dev_sn_len = OCLINK_MAX_SN_LEN;
            }
            else
            {
                g_dev_sn_len = len;
            }
            memcpy(g_dev_sn, (char *)data, g_dev_sn_len);
            break;
        case OCLINK_CMD_CALLBACK:
            los_cmd_func = (cmdcallback)data;
            break;
        default:
            break;
    }
    return 0;
}



int los_handle_coap_response(struct _coap_context_t *ctx, coap_msg_t *msg)
{
    if(COAP_MESSAGE_CON == msg->head.t
        && COAP_REQUEST_GET == msg->head.code
        && COAP_OPTION_OBSERVE == msg->option->optnum )
    {
        /* bind success */
        if (!g_bind_finsh)
        {
            memcpy(g_tok,msg->tok->token, msg->tok->tklen);
            g_tok_len = msg->tok->tklen;
            g_bind_finsh = 1;
        }
        else
        {
            g_tok_len = 0;
            g_bind_finsh = 0;
            los_oclink_set_errstate(&oclink_state);
        }
        return 0;
    }
    if(COAP_MESSAGE_RST == msg->head.t)
    {
        /* bind failed */
        g_bind_finsh = 0;
        memset(g_tok,0xf1,sizeof(g_tok));
        g_tok_len = 8;
        return 0;
    }
    if(COAP_MESSAGE_ACK == msg->head.t && 0xa0 == msg->head.code)
    {
        /* bind failed */
        los_oclink_set_errcode(OCLINK_ERR_CODE_DEVNOTEXSIT);
        return 0;
    }
    if(COAP_MESSAGE_ACK == msg->head.t && 0x44 == msg->head.code)
    {
        /* ok,get first back ack, wait another */
        return 0;
    }
	return 0;	
}

static const coap_res_path_t path_observe = {2, {"t", "d"}};
static const coap_res_path_t path_cmd = {2, {"t", "d"}};

static int los_handle_observe_request(coap_msg_t *rcvmsg, coap_msg_t *outmsg);
static int los_handle_cmd_request(coap_msg_t *rcvmsg, coap_msg_t *outmsg);
static coap_res_t g_oclink_res[] = 
{
    {COAP_REQUEST_GET, los_handle_observe_request, &path_observe, NULL},
    {COAP_REQUEST_POST, los_handle_cmd_request, &path_cmd, "ct=0"},
    {0, NULL, NULL, NULL}
};

int los_handle_wellknown_request(coap_msg_t *rcvmsg, coap_msg_t *outmsg)
{
    return 0;
}
int los_handle_rd_request(coap_msg_t *rcvmsg, coap_msg_t *outmsg)
{
    return 0;
}
int los_handle_observe_request(coap_msg_t *rcvmsg, coap_msg_t *outmsg)
{
    coap_option_t *opts = NULL;
    char contenttype[2] = {0};
    outmsg->head.msgid[0] = rcvmsg->head.msgid[0];
    outmsg->head.msgid[1] = rcvmsg->head.msgid[1];

    contenttype[0] = ((uint16_t)COAP_CONTENTTYPE_NONE & 0xFF00) >> 8;
    contenttype[1] = ((uint16_t)COAP_CONTENTTYPE_NONE & 0x00FF);
    opts = los_coap_add_option_to_list(opts, 
                                COAP_OPTION_CONTENT_FORMAT, 
                                (char *)contenttype, 2);
    los_coap_add_option(outmsg, opts);
    return 0;
}

int los_handle_cmd_request(coap_msg_t *rcvmsg, coap_msg_t *outmsg)
{
    coap_option_t *opts = NULL;
    char light = '0';
    char contenttype[2] = {0};
    outmsg->head.msgid[0] = rcvmsg->head.msgid[0];
    outmsg->head.msgid[1] = rcvmsg->head.msgid[1];

    contenttype[0] = ((uint16_t)COAP_CONTENTTYPE_TEXT_PLAIN & 0xFF00) >> 8;
    contenttype[1] = ((uint16_t)COAP_CONTENTTYPE_TEXT_PLAIN & 0x00FF);
    opts = los_coap_add_option_to_list(opts, 
                                COAP_OPTION_CONTENT_FORMAT, 
                                (char *)contenttype, 2);
    los_coap_add_option(outmsg, opts);
    los_coap_add_paylaod(outmsg, &light, 1);
    
    if (NULL != los_cmd_func)
    {
        los_cmd_func((char *)rcvmsg->payload, rcvmsg->payloadlen);
    }
    
    return 0;
}

static int los_oclink_set_nextstate(int *state)
{
    switch(*state)
    {
        case OCLINK_STAT_INIT:
            *state = OCLINK_STAT_BINDING;
            break;
        case OCLINK_STAT_BINDING:
            *state = OCLINK_STAT_WAIT_BINDOK;
            break;
        case OCLINK_STAT_WAIT_BINDOK:
            *state = OCLINK_STAT_IDLE;
            break;
        case OCLINK_STAT_IDLE:
            *state = OCLINK_STAT_IDLE;
            break;
        case OCLINK_STAT_ERR:
            *state = OCLINK_STAT_BINDING;
            break;
        default:
            *state = OCLINK_STAT_ERR;
            break;
    }
    return 0;
}

static int los_oclink_get_curstate(void *res)
{
    return oclink_state;
}
static int los_oclink_set_errstate(int *state)
{   
    if (NULL != state)
    {
        *state = OCLINK_STAT_ERR;
    }
    return 0;
}
static int los_oclink_set_state(int state)
{   
    if (state >= OCLINK_STAT_INIT && state < OCLINK_STAT_MAX)
    {
        oclink_state = state;
    }
    return 0;
}
int los_oclink_task()
{
	void *remoteser = NULL;
	coap_option_t *opts = NULL;
	coap_msg_t *msg = NULL;
	int ret  = 0;
    int startrcvmsg = 0;
    
    unsigned long long bindstarttm;
    unsigned long long curtm;
    
    los_coap_stack_init();
    
    while(1)
    {
        /* stop oclink */
        if (oclink_stop_flag)
        {
            if (ctx)
            {
                los_coap_free_context(ctx);
            }
            oclink_stop_flag = 0;
            break;
        }
        switch(los_oclink_get_curstate(NULL))
        {
            case OCLINK_STAT_INIT:
                remoteser = los_coap_new_resource(g_iot_server, g_iot_port);
                if(NULL == remoteser)
                {
                    los_coap_delay(100);
                    continue;
                }
                ctx = los_coap_malloc_context(remoteser);
                if(NULL == ctx)
                {
                    los_coap_delete_resource(remoteser);
                    los_coap_delay(100);
                    continue;
                }
                ret = los_coap_register_handler(ctx, los_handle_coap_response);
                if (ret < 0)
                {
                    los_coap_free_context(ctx);
                    ctx = NULL;
                    los_coap_delay(100);
                    continue;
                }
                los_coap_add_resource(ctx, g_oclink_res);
                los_oclink_set_nextstate(&oclink_state);
                break;
            case OCLINK_STAT_BINDING:
                if (g_dev_sn_len == 0)
                {
                    /* no sn data, user should set hte sn data. */
                    los_coap_delay(100);
                    continue;
                }
                g_tok_len = los_coap_generate_token((unsigned char *)g_tok);
                opts = los_coap_add_option_to_list(opts, 
                                            COAP_OPTION_URI_PATH, 
                                            (char *)oclink_res, 1);
                
                opts = los_coap_add_option_to_list(opts, 
                                            COAP_OPTION_URI_PATH, 
                                            (char *)oclink_res1, 1);
                memcpy(oclink_res2+3, g_dev_sn, g_dev_sn_len);
                opts = los_coap_add_option_to_list(opts, 
                                            COAP_OPTION_URI_QUERY, 
                                            (char *)oclink_res2, g_dev_sn_len + 3);
                msg = los_coap_new_msg(ctx,
                                        COAP_MESSAGE_CON, 
                                        COAP_REQUEST_POST, 
                                        opts, 
                                        NULL, 
                                        0);
                ret = los_coap_add_token(msg, g_tok, g_tok_len);
                if (ret < 0)
                {
                    los_coap_delete_msg(msg);
                    msg = NULL;
                    los_oclink_set_errstate(&oclink_state);
                    break;
                }
                /* send bind msg to huawei iot platform. */
                ret = los_coap_send(ctx, msg);
                if (ret < 0)
                {
                    /* send err */
                    break;
                }
                los_oclink_set_nextstate(&oclink_state);
                startrcvmsg = 1;
                bindstarttm = LOS_TickCountGet();
                break;
            case OCLINK_STAT_WAIT_BINDOK:
                curtm = LOS_TickCountGet();
                if (curtm - bindstarttm >= OCLINK_BIND_TIMEOUT)
                {
                    g_bind_finsh = 0;
                    los_oclink_set_state(OCLINK_STAT_BINDING);
                    startrcvmsg = 0;
                    los_oclink_set_errcode(OCLINK_ERR_CODE_BINDTIMEOUT);
                    los_coap_discard_resndqueue(ctx);
                    break;
                }
                if(g_bind_finsh)
                {
                    los_oclink_set_nextstate(&oclink_state);
                }
                break;
            case OCLINK_STAT_IDLE:
                if (NULL != ctx->sndque)
                {
                    ret = los_coap_send(ctx, ctx->sndque->msg);
                    los_coap_remove_sndqueue(ctx, ctx->sndque->msg);
                }
                break;
            case OCLINK_STAT_ERR:
            default:
                los_oclink_set_state(OCLINK_STAT_BINDING);
                startrcvmsg = 0;
                g_bind_finsh = 0;
                break;
        }
        /* oclink task should recive all coap messages. */
        if (startrcvmsg)
        {
            ret = los_coap_read(ctx);
            if (ret == -LOS_COAP_SOCKET_NETWORK_ERR)
            {
                los_oclink_set_errcode(OCLINK_ERR_CODE_NETWORK);
                los_coap_delay(100);
            }
        }
    }
    return 0;
}

int los_oclink_send(char *buf, int len)
{
    coap_msg_t *msg = NULL;
    coap_option_t *opts = NULL;
    int ret = 0;
    static unsigned char lifetime = 0;
    
    if (NULL == ctx)
    {
        return -1;
    }
    
    while(los_oclink_get_curstate(NULL) != OCLINK_STAT_IDLE)
    {
        ret++;
        los_coap_delay(10);
        if (ret >= OCLINK_MAX_SEND_RETRY)
        {
            return -1;
        }
    }
    lifetime++;
    opts = los_coap_add_option_to_list(opts, 
                                COAP_OPTION_OBSERVE, 
                                (char *)&lifetime, 1);
    msg = los_coap_new_msg(ctx,
                            COAP_MESSAGE_NON, 
                            LOS_COAP_RESP_205, 
                            opts, 
                            (unsigned char *)buf, 
                            len);
    if (NULL == msg)
    {
        los_coap_free_option(opts);
        opts = NULL;
        return -1;
    }
    ret = los_coap_add_token(msg, g_tok, g_tok_len);
    if (ret < 0)
    {
        los_coap_delete_msg(msg);
        msg = NULL;
        return -1;
    }
    ret = los_coap_send(ctx, msg);    
    return 0;
}

int los_oclink_start(void)
{
	UINT32 uwRet;
	TSK_INIT_PARAM_S stTaskInitParam;

	memset((void *)(&stTaskInitParam), 0, sizeof(TSK_INIT_PARAM_S));
	stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)los_oclink_task;
	stTaskInitParam.uwStackSize = OCLINK_TASK_STACK_SIZE;
	stTaskInitParam.pcName = "oclinktask";
	stTaskInitParam.usTaskPrio = OCLINK_TASK_PRIO;
    
	uwRet = LOS_TaskCreate(&g_oclink_taskid, &stTaskInitParam);
	if (uwRet != LOS_OK)
	{
		return -1;
	}
    
    return 0;
}

int los_oclink_stop(void)
{
    oclink_stop_flag = 1;
    /* wait oclink data process task finish recieve data and exit */
    los_coap_delay(1000);
    if(LOS_OK != LOS_TaskDelete(g_oclink_taskid))
    {
        return LOS_NOK;
    }
    g_oclink_taskid = 0;
    return 0;
}


