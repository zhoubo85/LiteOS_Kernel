
#ifdef LOS_STM32F746ZG
#include "stm32f7xx_hal.h"
#endif
#include "los_builddef.h"

#include "los_mip_config.h"
#include "los_mip_err.h"
#include "los_mip_netif.h"
#include "los_mip_ethif.h"
#include "los_mip_osfunc.h"
#include "los_mip_netbuf.h"
#include "los_mip_arp.h"
#include "los_mip_tcpip_core.h"

#include <string.h>

#define MIP_DEF_MAC0 0x02
#define MIP_DEF_MAC1 0x0A
#define MIP_DEF_MAC2 0x0F
#define MIP_DEF_MAC3 0x0E
#define MIP_DEF_MAC4 0x0D
#define MIP_DEF_MAC5 0x06

/* The time to block waiting for input. */
#define MIP_ETH_IN_WAIT_FOREVER     (0xffffffffUL)

#define MIP_ETH_TASK_STACK_SIZE     (1024)
#define MIP_ETH_TASK_PRIO           (7)

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

#ifdef LOS_STM32F746ZG
#if defined ( __CC_ARM ) /* MDK Compiler */

__align(4) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB];/* Ethernet Rx MA Descriptor */
__align(4) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Tx DMA Descriptor */
__align(4) uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE];/* Ethernet Receive Buffer */
__align(4) uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE];/* Ethernet Transmit Buffer */

#elif defined ( __ICCARM__ )/* IAR Compiler */

#pragma data_alignment=4
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB];/* Ethernet Rx MA Descriptor */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Tx DMA Descriptor */
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE];/* Ethernet Receive Buffer */
uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE];/* Ethernet Transmit Buffer */

#elif defined ( __GNUC__ )/* GNU Compiler */

ETH_DMADescTypeDef  DMARxDscrTab[ETH_RXBUFNB];/* Ethernet Rx MA Descriptor */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TXBUFNB];/* Ethernet Tx DMA Descriptor */
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE];/* Ethernet Receive Buffer */
uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE];/* Ethernet Transmit Buffer */

#endif

/* Global Ethernet handle */
ETH_HandleTypeDef EthHandle;

#endif /* LOS_STM32F746ZG */

#ifdef LOS_STM32F746ZG

static mip_sem_t g_eth_datsem;
static void los_mip_ethif_input_task( void * argument );

/**
  * @brief  Initializes the ETH MSP.
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{

    GPIO_InitTypeDef GPIO_InitStructure;
  
    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Ethernet pins configuration ************************************************/
    /*
        RMII_REF_CLK ----------------------> PA1
        RMII_MDIO -------------------------> PA2
        RMII_MDC --------------------------> PC1
        RMII_MII_CRS_DV -------------------> PA7
        RMII_MII_RXD0 ---------------------> PC4
        RMII_MII_RXD1 ---------------------> PC5
        RMII_MII_RXER ---------------------> PG2
        RMII_MII_TX_EN --------------------> PG11
        RMII_MII_TXD0 ---------------------> PG13
        RMII_MII_TXD1 ---------------------> PB13
    */

    /* Configure PA1, PA2 and PA7 */
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL; 
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
  
    /* Configure PB13 */
    GPIO_InitStructure.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
  
    /* Configure PC1, PC4 and PC5 */
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure PG2, PG11, PG13 and PG14 */
    GPIO_InitStructure.Pin =  GPIO_PIN_2 | GPIO_PIN_11 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);
  
    /* Enable the Ethernet global Interrupt */
    HAL_NVIC_SetPriority(ETH_IRQn, 0x7, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
  
    /* Enable ETHERNET clock  */
    __HAL_RCC_ETH_CLK_ENABLE();
}

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    (void)heth;
    los_mip_sem_signal(&g_eth_datsem);
}


/**
  * @brief In this function, the hardware should be initialized.
  * Called from los_mip_dev_eth_init().
  *
  * @param netif: initialized network interface structure
  *        for this ethernet interface
  */
static void los_mip_eth_hw_init(struct netif *netif)
{
    uint8_t macaddress[6]= { MIP_DEF_MAC0, MIP_DEF_MAC1, MIP_DEF_MAC2, MIP_DEF_MAC3, MIP_DEF_MAC4, MIP_DEF_MAC5 };
  
    EthHandle.Instance = ETH;  
    EthHandle.Init.MACAddr = macaddress;
    EthHandle.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    EthHandle.Init.Speed = ETH_SPEED_100M;
    EthHandle.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    EthHandle.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;
    EthHandle.Init.RxMode = ETH_RXINTERRUPT_MODE;
    EthHandle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
    EthHandle.Init.PhyAddress = LAN8742A_PHY_ADDRESS;
    
    /* create a binary semaphore used for informing ethernetif of frame reception */
    los_mip_sem_new(&g_eth_datsem, 1);
    los_mip_sem_wait(&g_eth_datsem, 0);
    
    /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
    if (HAL_ETH_Init(&EthHandle) == HAL_OK)
    {
        /* Set netif link flag */
        netif->flags |= NETIF_FLAG_LINK_UP; 
    }
  
    /* Initialize Tx Descriptors list: Chain Mode */
    HAL_ETH_DMATxDescListInit(&EthHandle, DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);
     
    /* Initialize Rx Descriptors list: Chain Mode  */
    HAL_ETH_DMARxDescListInit(&EthHandle, DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

    /* set netif MAC hardware address length */
    netif->hwaddr_len = MIP_ETH_HWADDR_LEN;

    /* set netif MAC hardware address */
    netif->hwaddr[0] =  MIP_DEF_MAC0;
    netif->hwaddr[1] =  MIP_DEF_MAC1;
    netif->hwaddr[2] =  MIP_DEF_MAC2;
    netif->hwaddr[3] =  MIP_DEF_MAC3;
    netif->hwaddr[4] =  MIP_DEF_MAC4;
    netif->hwaddr[5] =  MIP_DEF_MAC5;

    /* set netif maximum transfer unit */
    netif->mtu = MIP_HW_MTU_VLAUE;
    
    /* Accept broadcast address and ARP traffic */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    /* create the task that handles the ETH_MAC */
	los_mip_thread_new("MIP_ETH_INPUT", 
						los_mip_ethif_input_task , 
						(void *)netif,//NULL,/*void *arg,*/ 
						MIP_ETH_TASK_STACK_SIZE, 
						MIP_ETH_TASK_PRIO);

    /* Enable MAC and DMA transmission and reception */
    HAL_ETH_Start(&EthHandle);
}


/**
  * @brief This function do the actual transmission of the packet. The packet is
  * contained in the netbuf that is passed to the function. This netbuf
  * might be chained.
  *
  * @param netif: the network interface structure for this ethernet interface
  * @param p: the MAC packet to send (e.g. IP packet including MAC addresses and type)
  * @return MIP_OK if the packet could be sent
  * @note 
  */
static int los_mip_eth_xmit(struct netif *netif, struct netbuf *p)
{
    int errval;
    struct netbuf *q;
    uint8_t *buffer = (uint8_t *)(EthHandle.TxDesc->Buffer1Addr);
    __IO ETH_DMADescTypeDef *DmaTxDesc;
    uint32_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;

    DmaTxDesc = EthHandle.TxDesc;
    bufferoffset = 0;
  
    /* copy frame from netbuf to driver buffers */
    for(q = p; q != NULL; q = q->next)
    {
        /* Is this buffer available? If not, goto error */
        if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
        {
            errval = -MIP_ETH_ERR_USE;
            goto error;
        }
    
        /* Get bytes in current buffer */
        byteslefttocopy = q->len;
        payloadoffset = 0;
    
        /* Check if the length of data to copy is bigger than Tx buffer size*/
        while( (byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE )
        {
            /* Copy data to Tx buffer*/
            memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), (ETH_TX_BUF_SIZE - bufferoffset) );

            /* Point to next descriptor */
            DmaTxDesc = (ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);

            /* Check if the buffer is available */
            if((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
            {
                errval = -MIP_ETH_ERR_USE;
                goto error;
            }

            buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);

            byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
            payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
            framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
            bufferoffset = 0;
        }
    
    /* Copy the remaining bytes */
    memcpy( (uint8_t*)((uint8_t*)buffer + bufferoffset), (uint8_t*)((uint8_t*)q->payload + payloadoffset), byteslefttocopy );
    bufferoffset = bufferoffset + byteslefttocopy;
    framelength = framelength + byteslefttocopy;
  }

    /* Prepare transmit descriptors to give to DMA */ 
    HAL_ETH_TransmitFrame(&EthHandle, framelength);

    errval = MIP_OK;
  
error:
  
    /* When Transmit Underflow flag is set, clear it and issue a Transmit Poll Demand to resume transmission */
    if ((EthHandle.Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET)
    {
        /* Clear TUS ETHERNET DMA flag */
        EthHandle.Instance->DMASR = ETH_DMASR_TUS;

        /* Resume DMA transmission*/
        EthHandle.Instance->DMATPDR = 0;
    }
    return errval;
}


/**
  * @brief Should allocate a netbuf and transfer the bytes of the incoming
  * packet from the interface into the netbuf.
  *
  * @param netif the network interface structure for this ethernet interface
  * @return a netbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct netbuf * los_mip_eth_pktin(struct netif *netif)
{
    struct netbuf *p = NULL, *q = NULL;
    uint16_t len = 0;
    uint8_t *buffer;
    __IO ETH_DMADescTypeDef *dmarxdesc;
    uint32_t bufferoffset = 0;
    uint32_t payloadoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t i=0;
  
    /* get received frame */
    if(HAL_ETH_GetReceivedFrame_IT(&EthHandle) != HAL_OK)
        return NULL;
  
    /* Obtain the size of the packet and put it into the "len" variable. */
    len = EthHandle.RxFrameInfos.length;
    buffer = (uint8_t *)EthHandle.RxFrameInfos.buffer;
  
    if (len > 0)
    {
        p = netbuf_alloc(NETBUF_RAW, len);
        if (NULL == p)
        {
            len = 0;
        }
    }

    if (p != NULL)
    {
        dmarxdesc = EthHandle.RxFrameInfos.FSRxDesc;
        bufferoffset = 0;

        for(q = p; q != NULL; q = q->next)
        {
            byteslefttocopy = q->len;
            payloadoffset = 0;

            /* Check if the length of bytes to copy in current netbuf is bigger than Rx buffer size */
            while( (byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE )
            {
                /* Copy data to netbuf */
                memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), (ETH_RX_BUF_SIZE - bufferoffset));

                /* Point to next descriptor */
                dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
                buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);

                byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
                payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
                bufferoffset = 0;
            }

            /* Copy remaining data in netbuf */
            memcpy( (uint8_t*)((uint8_t*)q->payload + payloadoffset), (uint8_t*)((uint8_t*)buffer + bufferoffset), byteslefttocopy);
            bufferoffset = bufferoffset + byteslefttocopy;
        }
    }
    
    /* Release descriptors to DMA */
    /* Point to first descriptor */
    dmarxdesc = EthHandle.RxFrameInfos.FSRxDesc;
    /* Set Own bit in Rx descriptors: gives the buffers back to DMA */
    for (i=0; i< EthHandle.RxFrameInfos.SegCount; i++)
    {  
        dmarxdesc->Status |= ETH_DMARXDESC_OWN;
        dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
    }
    
    /* Clear Segment_Count */
    EthHandle.RxFrameInfos.SegCount =0;

    /* When Rx Buffer unavailable flag is set: clear it and resume reception */
    if ((EthHandle.Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET)  
    {
        /* Clear RBUS ETHERNET DMA flag */
        EthHandle.Instance->DMASR = ETH_DMASR_RBUS;
        /* Resume DMA reception */
        EthHandle.Instance->DMARPDR = 0;
    }
    return p;
}

/**
  * @brief This function is the ethernet data input task, it is processed when a packet 
  * is ready to be read from the interface. It uses the function los_mip_eth_pktin() 
  * that should handle the actual reception of bytes from the network
  * interface. Then the type of the received packet is determined and
  * the appropriate input function is called.
  *
  * @param argument the network interface structure for this ethernet interface
*/
void los_mip_ethif_input_task(void * argument)
{
    struct netbuf *p;
    struct netif *netif = (struct netif *) argument;
  
    for( ;; )
    {
        if (NULL == netif)
        {
            los_mip_delay(20);
            continue;
        }
        if(los_mip_sem_wait(&g_eth_datsem, MIP_ETH_IN_WAIT_FOREVER) != MIP_OS_TIMEOUT)
        {
            do
            {
                p = los_mip_eth_pktin( netif );
                if (p != NULL)
                {
                    if (netif->input( p, netif) != MIP_OK )
                    {
                        netbuf_free(p);
                    }
                }
            }while(p!=NULL);
        }
    }
}

#endif
/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function los_mip_eth_hw_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to los_mip_netif_add().
  *
  * @param netif the network interface structure for this ethernet interface
  * @return MIP_OK if the loopif is initialized
  *         
  */
int los_mip_dev_eth_init(struct netif *netif)
{
    if (NULL == netif)
    {
        return -MIP_ETH_ERR_PARAM;
    }
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    
#ifdef LOS_STM32F746ZG
    netif->arp_xmit = los_mip_arp_xmit; 
    netif->hw_xmit = los_mip_eth_xmit; 
    netif->input = los_mip_tcpip_inpkt;
    /* initialize the hardware */
    los_mip_eth_hw_init(netif);
#endif
    
    return MIP_OK;
}
