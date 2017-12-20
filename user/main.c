#include "los_sys.h"
#include "los_tick.h"
#include "los_task.ph"
#include "los_config.h"

#include "los_bsp_led.h"
#include "los_bsp_key.h"
#include "los_bsp_uart.h"
#include "los_inspect_entry.h"
#include "los_demo_entry.h"

#define LOS_MIP_HAL_TEST 1
#define LOS_MIP_TEST_XXX 1

#include "cmsis_os.h"
#include "main.h"

#include "los_mip_netif.h"
#include "los_mip_tcpip_core.h"
#include "los_mip_ethif.h"
#include "los_mip_ipaddr.h"
#include "los_mip_ipv4.h"
#include "los_mip_socket.h"
#include "los_mip_inet.h"
#include "los_mip_dhcp.h"
#include "los_oclink_demo.h"


struct netif g_stm32f746;

static void los_config_network(void)
{
    ip_addr_t testip;
    ip_addr_t testmask;
    ip_addr_t testgw;
    
    los_mip_make_ipaddr(&testip,192,168,1,111);
    los_mip_make_ipaddr(&testmask, 255,255,255,0);
    los_mip_make_ipaddr(&testgw, 192,168,1,1);
    
    //init tcp/ip stack
    los_mip_tcpip_init(NULL);
    //init ethernet hardware
    los_mip_dev_eth_init(&g_stm32f746);
    //config ethernet ip address info and 
    los_mip_netif_config(&g_stm32f746, &testip, &testmask, &testgw);
    los_mip_netif_up(&g_stm32f746);
    los_mip_set_default_if(&g_stm32f746);
    
    return ;
}
char testbuf[200] = {0};
static UINT32 g_testtcp;
LITE_OS_SEC_TEXT VOID los_mip_test_tcp(void)
{
    int test = 0;
    int fd = -1;
    int clientfd = -1; 
    int port = 5000; 
    int rport = 6000;
    int clientlen = 0; 
    struct sockaddr_in servaddr;  
    struct sockaddr_in cliaddr; 
    int on = 1;
    fd = los_mip_socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0)
    {
        memset(&servaddr, 0, sizeof(servaddr));  
        memset(&cliaddr, 0, sizeof(cliaddr));  
        servaddr.sin_family = AF_INET;  
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
        servaddr.sin_port = htons((unsigned short)port);  
        clientlen = sizeof(cliaddr); 
        los_mip_bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        #if 0
        cliaddr.sin_family = AF_INET;  
        cliaddr.sin_addr.s_addr = inet_addr("192.168.137.1");  
        cliaddr.sin_port = htons((unsigned short)rport); 
        test = los_mip_connect(fd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
        if (test != 0)
        {
            while(1);
        }
        while(1)
        {
            test++;
            testbuf[0] = test + 1;
            testbuf[1] = test + 2;
            testbuf[2] = test + 3;
            testbuf[3] = test + 4;
            testbuf[4] = test + 5;
            test = los_mip_read(fd, testbuf, 50);
            los_mip_write(fd, testbuf, test);
            //LOS_TaskDelay(500);
        }
        //LOS_TaskDelay(25000);
        los_mip_close(fd);
        //los_mip_read(fd, testbuf, 50);
        #else
        los_mip_listen(fd, 0);
        memset(&cliaddr, 0, sizeof(cliaddr));
        LOS_TaskDelay(5000);

        clientfd = los_mip_accept(fd, (struct sockaddr*)&cliaddr, (socklen_t *)&clientlen);
        //los_mip_setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on)); 
        while(1)
        {
//            if (clientfd >= 0)
//                {
//                    test ++;
//                }
            test = los_mip_read(clientfd, testbuf, 50);
            los_mip_write(clientfd, testbuf, test);
            //LOS_TaskDelay(5000);
        }
        #endif
    }
}

LITE_OS_SEC_TEXT VOID los_mip_test_tcp_persist(void)
{
    int test = 0;
    int fd = -1;
    int clientfd = -1; 
    int port = 5000; 
    int rport = 6000;
    int clientlen = 0; 
    struct sockaddr_in servaddr;  
    struct sockaddr_in cliaddr; 
    int on = 1;
    fd = los_mip_socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0)
    {
        memset(&servaddr, 0, sizeof(servaddr));  
        memset(&cliaddr, 0, sizeof(cliaddr));  
        servaddr.sin_family = AF_INET;  
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
        servaddr.sin_port = htons((unsigned short)port);  
        clientlen = sizeof(cliaddr); 
        los_mip_bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));

        cliaddr.sin_family = AF_INET;  
        cliaddr.sin_addr.s_addr = inet_addr("192.168.1.101");  
        cliaddr.sin_port = htons((unsigned short)rport); 
        test = los_mip_connect(fd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
        if (test != 0)
        {
            while(1);
        }
        while(1)
        {
            test++;
            testbuf[0] = test + 1;
            testbuf[1] = test + 2;
            testbuf[2] = test + 3;
            testbuf[3] = test + 4;
            testbuf[4] = test + 5;
            //test = los_mip_read(fd, testbuf, 50);
            los_mip_write(fd, testbuf, 200);
            //LOS_TaskDelay(500);
        }
        //LOS_TaskDelay(25000);
        los_mip_close(fd);
        //los_mip_read(fd, testbuf, 50);

    }
}


LITE_OS_SEC_TEXT VOID los_mip_test_mulcast(void)
{
    int test = 0;
    int fd = -1;
    int clientfd = -1; 
    int port = 5000; 
    int rport = 6000;
    int clientlen = 0; 
    struct sockaddr_in servaddr;    
    
    struct ip_mreq testmreq;
    int ret = 0;
    int on = 1;
    fd = los_mip_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd >= 0)
    {
        memset(&servaddr, 0, sizeof(servaddr));  
        servaddr.sin_family = AF_INET;  
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
        servaddr.sin_port = htons((unsigned short)port);  

        los_mip_bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
        testmreq.imr_multiaddr.S_un.S_addr = inet_addr("224.0.0.88");
        testmreq.imr_interface.S_un.S_addr = inet_addr("192.168.1.111");
        ret = los_mip_setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &testmreq, sizeof (testmreq));

        while(1)
        {
            test = los_mip_read(fd, testbuf, 50);
        }
        
    }
}

LITE_OS_SEC_TEXT VOID los_mip_test_udpsnd(void)
{
    int test = 0;
    int fd = -1;
    int clientfd = -1; 
    int port = 5000; 
    int rport = 6000;
    int clientlen = 0; 
    struct sockaddr_in servaddr;  
    struct sockaddr_in locaaddr;
    int ret = 0;

    fd = los_mip_socket(AF_INET, SOCK_DGRAM, 0);
    if (fd >= 0)
    {
        memset(&servaddr, 0, sizeof(servaddr));  
        servaddr.sin_family = AF_INET;  
        servaddr.sin_addr.s_addr = inet_addr("192.168.1.102");  
        servaddr.sin_port = htons((unsigned short)port);  

        memset(&locaaddr, 0, sizeof(locaaddr));  
        locaaddr.sin_family = AF_INET;  
        locaaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
        locaaddr.sin_port = htons((unsigned short)port);  
        
        los_mip_bind(fd, (struct sockaddr*)&locaaddr, sizeof(locaaddr));
        while(1)
        {
            test = los_mip_sendto(fd, testbuf, 10, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            LOS_TaskDelay(5000);
        }
        
    }
}

void los_mip_tcp_task(void)
{
	UINT32 uwRet;
	TSK_INIT_PARAM_S stTaskInitParam;

	(VOID)memset((void *)(&stTaskInitParam), 0, sizeof(TSK_INIT_PARAM_S));
	//stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)los_mip_test_tcp;
    //stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)los_mip_test_mulcast;
    //stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)los_mip_test_udpsnd;
    stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)los_mip_test_tcp_persist;
	stTaskInitParam.uwStackSize = 1024;
	stTaskInitParam.pcName = "tcptest";
	stTaskInitParam.usTaskPrio = 10;
	uwRet = LOS_TaskCreate(&g_testtcp, &stTaskInitParam);

	if (uwRet != LOS_OK)
	{
		return ;
	}
	return ;
}

void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct; 
  RCC_OscInitTypeDef RCC_OscInitStruct; 
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9; 
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1) {};
  }
  
  /* Activate the OverDrive to reach the 216 Mhz Frequency */
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    while(1) {};
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    while(1) {};
  }
}

/**
  * @brief  Configures EXTI lines 15 to 10 (connected to PC.13 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTI15_10_IRQHandler_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOC clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Configure PC.13 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* Enable and set EXTI lines 15 to 10 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static UINT32 g_uwboadTaskID;
LITE_OS_SEC_TEXT VOID LOS_BoadExampleTskfunc(VOID)
{
	while (1)
	{
		LOS_EvbLedControl(LOS_LED2, LED_ON);
		LOS_EvbUartWriteStr("Board Test\n");
		LOS_TaskDelay(500);
		LOS_EvbLedControl(LOS_LED2, LED_OFF);
		LOS_TaskDelay(500);
	}
}
void LOS_BoadExampleEntry(void)
{
	UINT32 uwRet;
	TSK_INIT_PARAM_S stTaskInitParam;

	(VOID)memset((void *)(&stTaskInitParam), 0, sizeof(TSK_INIT_PARAM_S));
	stTaskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)LOS_BoadExampleTskfunc;
	stTaskInitParam.uwStackSize = LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE;
	stTaskInitParam.pcName = "BoardDemo";
	stTaskInitParam.usTaskPrio = 10;
	uwRet = LOS_TaskCreate(&g_uwboadTaskID, &stTaskInitParam);

	if (uwRet != LOS_OK)
	{
		return ;
	}
	return ;
}

/*****************************************************************************
 Function    : main
 Description : Main function entry
 Input       : None
 Output      : None
 Return      : None
 *****************************************************************************/
LITE_OS_SEC_TEXT_INIT
int main(void)
{
	UINT32 uwRet;


  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
	*/
    HAL_Init();
    /* Configure the system clock to 216 MHz */
    SystemClock_Config();

	/*Init LiteOS kernel */
	uwRet = LOS_KernelInit();
	if (uwRet != LOS_OK) 
	{
		return LOS_NOK;
	}
	/* Enable LiteOS system tick interrupt */
	LOS_EnableTick();
	
	/* 
		Notice: add your code here
		here you can create task for your function 
		do some hw init that need after systemtick init
	*/

	/* -2- Configure EXTI15_10 (connected to PC.13 pin) in interrupt mode */
	EXTI15_10_IRQHandler_Config();


    los_config_network();
    los_mip_tcp_task();
//	los_oclink_demo_start();
    
	/* Kernel start to run */
	LOS_Start();
	for (;;);
	/* Replace the dots (...) with your own code.  */
}
