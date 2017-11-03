
#include "los_bsp_led.h"
#include "los_task.h"
#ifdef LOS_ENABLE_MAX7299
#include "max7219_driver_func.h"
#endif

#include "los_oclink.h"
#include "los_oclink_demo.h"

static int test_led_status = 0;/* 0 led off, 1 led on */
static UINT32 g_oclink_demo;

int get_cmd;
int uset_value;
int scroll;

void led_set_status(void)
{
	test_led_status = (test_led_status + 1)%2;
	if(test_led_status)
	{
		LOS_EvbLedControl(LOS_LED3, LED_ON);
	}
	else
	{
		LOS_EvbLedControl(LOS_LED3, LED_OFF);
	}
}

int led_get_status(void)
{
	return test_led_status;
}

char max7219_get_show_data(int idx)
{
	if (idx < 0  || idx > 38)
	{
		return '0';
	}
	if (0<=idx && 9>= idx)
	{
		return 0x30+idx;
	}
	if (10<=idx || 35>=idx)
	{
		return 0x41 + idx - 10;
	}
	return '0';
}


int LOS_OceanCon_Init(void)
{

	return 0;
}
void ocean_report_cmd_status(int dataflag, int result)
{
    unsigned char data[8] = {0};
    data[0] = 0x4c;
    data[1] = 0x45;
    data[2] = 0x44;/* data[0`2] header */
    data[3] = 0x03;/* data report */
    if (dataflag == 1)
    {
        data[4] = 0x0d;/* set char index cmd */
    }
    else
    {
        data[4] = 0x03;/* set char scroll cmd */
    }
    if (result == 0)
    {
        data[5] = 0x00;
    }
    else
    {
        data[5] = 0x01;
    }
    los_oclink_send((char *)data, 6);
}


void oclink_recv_cmd(unsigned char *pucval, int nlen)
{
    #ifdef LOS_ENABLE_MAX7299
	char showd;
    #endif
    static int x = 0 ;
    
	/* here we should get the cmd and control the led_status */

	if (NULL == pucval || 0 == nlen)
	{
		return ;
	}
    if (4 == nlen && pucval[0] == 0xaa && pucval[1] == 0xaa && pucval[2] == 0x00 && pucval[3] == 0x00)
    {
        /* iot platform returned data , should discard. */
        x++;
        return ;
    }
    if (6 == nlen && pucval[0] == 0x4c && pucval[1] == 0x45 && pucval[2] == 0x44)
    {
        if (pucval[3] == 0x03)
        {
            /* cmd comes */
            switch(pucval[4]) 
            {
                case 0x0d:/* set showed char */ 
                    if (pucval[5] > 35)
                    {
                        uset_value = 0;
                        ocean_report_cmd_status(1, 1);
                        break;
                    }
                    uset_value = pucval[5];
                    #ifdef LOS_ENABLE_MAX7299
                    showd = max7219_get_show_data(uset_value);
                    LOS_TaskLock();
                    LOS_MAX7219_show(showd);
                    LOS_TaskUnlock();
                    #endif
                    ocean_report_cmd_status(1, 0);
                    break;
                case 0x03:/* set char scroll */
                    if (pucval[5] == 0x0)
                    {
                        scroll = 0;
                    }
                    else
                    {
                        scroll = 1;
                    }
                    ocean_report_cmd_status(0, 0);
                    break;
                default:
                    break;
            }
            /* need send cmd status */
        }
        else
        {
            /* not correct cmd */
            return ;
        }
    }
    
	return ;
}

void los_oclink_demo_task(void * pvparameters)
{
    int ret = 0;
	unsigned char data[8] = {0};
	int testd = 0;
    
    #ifdef LOS_ENABLE_MAX7299
	char showd;
    #endif
    unsigned short iotports = 5683;

    (void)pvparameters;
    
    ret = los_oclink_set_param(OCLINK_IOTS_ADDR, "112.93.129.154", strlen("112.93.129.154"));
    ret = los_oclink_set_param(OCLINK_IOTS_PORT, &iotports, 0);
    /*
        ret = los_oclink_set_param(OCLINK_IOTS_ADDRPORT, "112.93.129.154,5683", strlen("112.93.129.154,5683"));
        ret = los_oclink_set_param(OCLINK_IOT_DEVSN, "test00000002", 12);
    */
    ret = los_oclink_set_param(OCLINK_IOT_DEVSN, "liteos000002", 12);
    ret = los_oclink_set_param(OCLINK_CMD_CALLBACK, (void *)oclink_recv_cmd, 0);
    ret = los_oclink_start();
    
    /* just demo how to use oclink interface */
    if (ret < 0)
    {
        ret = los_oclink_stop();
        return ;
    }
    
	/* Init led matrix hardware driver */
    #ifdef LOS_ENABLE_MAX7299
	LOS_TaskLock();
	LOS_MAX7219_Init();
	LOS_TaskUnlock();
    #endif
	
	//wait socket ok
	LOS_TaskDelay(5000);

	while(1)
	{
		data[0] = 0x4c;
		data[1] = 0x45;
		data[2] = 0x44;/* data[0`2] header */
		data[3] = 0x0d;/* data report */ 
		data[4] = 0x00;/* led on */
        if (scroll == 0)
        {
            /* not scroll */
            data[5] = 0x0;
            testd = uset_value;
        }
        else
        {
            /* scroll */
            uset_value = testd;
            testd = (testd + 1)%36;
            data[5] = 0x1;
        }
        data[4] = testd;
        #ifdef LOS_ENABLE_MAX7299
        showd = max7219_get_show_data(data[4]);
        LOS_TaskLock();
        LOS_MAX7219_show(showd);
        LOS_TaskUnlock();
        #endif
        los_oclink_send((char *)data, 6);
		LOS_TaskDelay(5000);
        

	}

}


void los_oclink_demo_start(void)
{
	UINT32 uwRet;
	TSK_INIT_PARAM_S stTaskInitParam;

	memset((void *)(&stTaskInitParam), 0, sizeof(TSK_INIT_PARAM_S));
	stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)los_oclink_demo_task;
	stTaskInitParam.uwStackSize = OCLINK_DEMO_TASK_STACK_SIZE;
	stTaskInitParam.pcName = "OCLINK_DEMO";
	stTaskInitParam.usTaskPrio = OCLINK_DEMO_TASK_PRIO;
    
	uwRet = LOS_TaskCreate(&g_oclink_demo, &stTaskInitParam);
	if (uwRet != LOS_OK)
	{
		return ;
	}
}
