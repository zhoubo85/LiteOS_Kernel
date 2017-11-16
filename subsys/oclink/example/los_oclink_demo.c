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
 
#include "los_task.h"
#include "los_bsp_led.h"
#include "los_oclink.h"
#include "los_oclink_demo.h"

#ifdef LOS_ENABLE_MAX7299
#include "max7219_driver_func.h"
#endif

/* 0 led off, 1 led on */
static int test_led_status = 0;
/* oclink task handle */
static UINT32 g_oclink_demo;

/* cmd process global var */
static int uset_value;
static int scroll;

/*****************************************************************************
 Function    : led_set_status
 Description : set led3 on/off
 Input       : None
 Output      : None
 Return      : None
 *****************************************************************************/
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

/*****************************************************************************
 Function    : led_get_status
 Description : get led3 on/off status
 Input       : None
 Output      : None
 Return      : test_led_status @ current led status
 *****************************************************************************/
int led_get_status(void)
{
	return test_led_status;
}

/*****************************************************************************
 Function    : max7219_get_show_data
 Description : get the char which will display on max7219 by index 
 Input       : idx @ the char's index
 Output      : None
 Return      : char @ the char that will display on max7219
 *****************************************************************************/
char max7219_get_show_data(int idx)
{
	if ((idx < 0)  || (idx > 38))
	{
		return '0';
	}
	if ((0 <= idx) && (9 >= idx))
	{
		return 0x30+idx;
	}
	if ((10 <= idx) || (35 >= idx))
	{
		return 0x41 + idx - 10;
	}
	return '0';
}

/*****************************************************************************
 Function    : ocean_report_cmd_status
 Description : report cmd processing status
 Input       : dataflag @ flag for if the cmd is set char index or set scroll
               flag.
               result @ flag that indicate cmd proccess ok or ng
 Output      : None
 Return      : None
 *****************************************************************************/
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

/*****************************************************************************
 Function    : oclink_recv_cmd
 Description : process cmd that iot server send to us
 Input       : pucval @ cmd data buf
               nlen @ recived data length
 Output      : None
 Return      : None
 *****************************************************************************/
void oclink_recv_cmd(unsigned char *pucval, int nlen)
{
    #ifdef LOS_ENABLE_MAX7299
	char showd;
    #endif
    /* used for test */
    static int x = 0;
    
	/* here we should get the cmd and control the led_status */
	if ((NULL == pucval) || (0 == nlen))
	{
		return ;
	}
    if ((4 == nlen) && (pucval[0] == 0xaa) 
        && (pucval[1] == 0xaa) && (pucval[2] == 0x00) 
        && (pucval[3] == 0x00))
    {
        /* iot platform returned data , should discard. */
        x++;
        return ;
    }
    if ((6 == nlen) && (pucval[0] == 0x4c) 
        && (pucval[1] == 0x45) && (pucval[2] == 0x44))
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

/*****************************************************************************
 Function    : los_oclink_demo_task
 Description : oclink demo for how to use oclink 
 Input       : pvparameters @ unused
 Output      : None
 Return      : None
 *****************************************************************************/
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
    
    ret = los_oclink_set_param(OCLINK_IOTS_ADDR, "112.93.129.154", 
                               strlen("112.93.129.154"));
    ret = los_oclink_set_param(OCLINK_IOTS_PORT, &iotports, 0);
    /*
        ret = los_oclink_set_param(OCLINK_IOTS_ADDRPORT, "112.93.129.154,5683", 
                                   strlen("112.93.129.154,5683"));
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

/*****************************************************************************
 Function    : los_oclink_demo_start
 Description : create oclink demo task
 Input       : None
 Output      : None
 Return      : None
 *****************************************************************************/
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
