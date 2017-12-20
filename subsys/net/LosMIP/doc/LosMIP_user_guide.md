## LosMIP简介
LosMIP是专门为LiteOS开发的一款micro tcp/ip协议栈。主要用于ram资源受限的嵌入式环境下(比如只有几十K的ram的芯片)。

## LosMIP支持的特性

目前LosMIP支持以下协议

    1: arp
    2: ipv4（fragment not supported yet）
    3: icmp( echo )
    4: udp
    5: compatible socket interface , but just a few params supported
    6: simple dhcp client

## LosMIP代码简介

core ： 本文件夹下存放协议的核心代码实现

driver： 存放网口的驱动程序

include：存放协议使用的所有头文件

sys：存放于操作系统密切相关的函数的实现文件，比如内存池管理，消息队列，任务管理、信号量等等。

## socket使用
- LosMIP 的使用和linux上的socket接口使用方式是一样的，如果不熟悉linux socket编程，请先熟悉linux的socket编程，在网络上有大量的参考资料。不过LosMIP的头文件与linux的socket头文件不同，请参考后续章节。
- LosMIP的接口与linux下的socket接口相比，LosMIP的接口均是以los_mip_ 这样的前缀开头。

- 头文件包含。使用LosMIP编程，需要包含如下头文件
		
		#include "los_mip_netif.h"
		#include "los_mip_tcpip_core.h"
		#include "los_mip_ethif.h"
		#include "los_mip_ipaddr.h"
		#include "los_mip_socket.h"
		#include "los_mip_inet.h"
		//#include "los_mip_dhcp.h"

- 初始化协议栈以及挂载网口驱动

  使用网络协议栈之前需要初始化网口以及网络协议栈，示例代码如下。

		struct netif g_stm32f746;
		ip_addr_t testip;
		ip_addr_t testmask;
		ip_addr_t testgw;
    
		los_mip_make_ipaddr(&testip,192,168,137,2);
		los_mip_make_ipaddr(&testmask, 255,255,255,0);
		los_mip_make_ipaddr(&testgw, 192,168,137,1);
    
		/* init tcp/ip stack */
		los_mip_tcpip_init(NULL);
		/* init ethernet hardware */
		los_mip_dev_eth_init(&g_stm32f746);
		/* config ethernet ip address info */
		los_mip_netif_config(&g_stm32f746, &testip, &testmask, &testgw);
		los_mip_netif_up(&g_stm32f746);
		los_mip_set_default_if(&g_stm32f746);

### UDP socket 使用
- 使用的步骤如下

  1：创建udp socket

  2：bind一个本地接口（注：步骤为可选步骤，可以不执行）
  
  3：设置socket的工作模式（比如非阻塞模式，默认为阻塞模式）（注：步骤为可选步骤，可以不执行）

  4：调用 读写接口进行读写操作。

  说明：mip的使用和linux下的socket的接口使用类似，请参考linux的socket编程相关内容。

- 创建socket并接收发送数据，示例如下

		sxx = los_mip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 

		int iMode = 1;  
		ret = los_mip_ioctlsocket(sxx,FIONBIO,&iMode); 
    
		memset(&toAddr, 0, sizeof(toAddr));
		toAddr.sin_family =  AF_INET;

		memset(&localaddr, 0, sizeof(localaddr));
		localaddr.sin_family = AF_INET;
		localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		localaddr.sin_port = htons(5000);
	
		los_mip_bind(sxx, (struct sockaddr *)&localaddr, sizeof(localaddr));
		toAddr.sin_addr.s_addr = inet_addr("192.168.137.1");

		toAddr.sin_port = htons(4000);
    

		while(1)
		{
    		FD_SET(sxx, &rfd);
			if(los_mip_select(sxx+1,&rfd,0,0, &timeout)==0)
			{
            
			}
			else
			{
				n = los_mip_recv(sxx, tx, 2, 0);
				n = los_mip_sendto(sxx,tx, 2, 0,(struct sockaddr *)&toAddr,sizeof(struct sockaddr_in));
        }
    }

### TCP socket使用

- 使用的步骤如下

  1：创建tcp socket

  2：bind一个本地接口
  
  3：设置socket的工作模式（比如非阻塞模式，默认为阻塞模式）（注：步骤为可选步骤，可以不执行）

  4：调用accept接口或者connect接口进行连接

  5：调用 读写接口进行读写操作。

  说明：mip的使用和linux下的socket的接口使用类似，请参考linux的socket编程相关内容。

- 简单创建tcp client并接收发送数据，示例如下

		int iMode = 1;  
		fd = los_mip_socket(AF_INET, SOCK_STREAM, 0);
		if (fd >= 0)
		{
			/* set noblock mode, if don't set default is block mode */
			ret = los_mip_ioctlsocket(fd,FIONBIO,&iMode); 
        	memset(&localaddr, 0, sizeof(localaddr));  
        	memset(&remoteaddr, 0, sizeof(remoteaddr));  
        	localaddr.sin_family = AF_INET;  
        	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
        	localaddr.sin_port = htons((unsigned short)port);  
        	clientlen = sizeof(cliaddr); 
			/* bind to a local port for the socket, if don't do this system
			   will generate a random unused port for socket */
        	los_mip_bind(fd, (struct sockaddr*)&localaddr, sizeof(localaddr));

        	remoteaddr.sin_family = AF_INET;  
        	remoteaddr.sin_addr.s_addr = inet_addr("192.168.137.1");  
        	remoteaddr.sin_port = htons((unsigned short)rport); 
			/* connect remote server */
        	test = los_mip_connect(fd, (struct sockaddr*)&remoteaddr, sizeof(remoteaddr));
        	if (test != 0)
        	{
            while(1);
        	}
			while(1)
			{
				/* read / write data  */
            	test = los_mip_read(fd, testbuf, 50);
            	los_mip_write(fd, testbuf, test);
			}
		}

- 简单创建tcp server并接收发送数据，示例如下

		int iMode = 1;  
		fd = los_mip_socket(AF_INET, SOCK_STREAM, 0);
		if (fd >= 0)
		{
			/* set noblock mode, if don't set default is block mode */
			ret = los_mip_ioctlsocket(fd,FIONBIO,&iMode); 
        	memset(&localaddr, 0, sizeof(localaddr));  
        	memset(&remoteaddr, 0, sizeof(remoteaddr));  
        	localaddr.sin_family = AF_INET;  
        	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
        	localaddr.sin_port = htons((unsigned short)port);  
        	clientlen = sizeof(cliaddr); 
			/* bind to a local port for the socket, if don't do this system
			   will generate a random unused port for socket */
        	los_mip_bind(fd, (struct sockaddr*)&localaddr, sizeof(localaddr));
			/* listen the port */
        	los_mip_listen(fd, backlog);
        	memset(&remoteaddr, 0, sizeof(remoteaddr));
			/* connect remote server */
        	 clientfd = los_mip_accept(fd, (struct sockaddr*)&remoteaddr, (socklen_t *)&remoteaddr);
        	if (clientfd < 0)
        	{
            	while(1);
        	}
			while(1)
			{
				/* read / write data  */
            	test = los_mip_read(clientfd, testbuf, 50);
            	los_mip_write(clientfd, testbuf, test);
				if(/* all things done*/)
				{
					break;
				}
			}
			los_mip_close(clientfd);
			los_mip_close(fd);
		}

- tcp 目前支持设置的option

		function：int los_mip_fcntl(int s, int cmd, int val)
        cmd：
          F_SETFL
        val：
          O_NONBLOCK

		function： int los_mip_setsockopt(int s, int level, 
                       int optname, 
                       const void *optval, 
                       socklen_t optlen) 
		level：
        SOL_SOCKET
          optname：
          SO_RCVTIMEO
		IPPROTO_TCP
          optname：
		  TCP_NODELAY
          TCP_QUICKACK


		function：int los_mip_setsockopt(int s, int level, 
                       int optname, 
                       const void *optval, 
                       socklen_t optlen)
		level：
		SOL_SOCKET
			optname：
            SO_RCVTIMEO
		IPPROTO_TCP
			optname：
			TCP_NODELAY
			TCP_QUICKACK
		IPPROTO_IP
			optname：
			IP_MULTICAST_TTL
			IP_MULTICAST_IF
			IP_ADD_MEMBERSHIP
			IP_DROP_MEMBERSHIP

## LosMIP网口驱动添加

- 添加一个网口驱动需要实现如下接口

		/* 
			通过这个接口实现网口的硬件初始化， 并且填充netif的 arp_xmit，hw_xmit， input等接口的
            其中：arp_xmit填充为los_mip_arp_xmit
				hw_xmit 设置为网口的发送接口
                input 设置为los_mip_tcpip_inpkt

			其他说明：收到网络数据包后，调用netif->input()接口将数据传递给LosMIP。该过程一般是通过
            创建一个task并结合网口的中断来实现的。
		*/
		int los_mip_dev_eth_init(struct netif *netif)

示例代码可以参考driver目录下的los_mip_ethif.c

## LosMIP移植到其他操作系统
- 移植LosMIP到其他操作系统的时候，需要将sys目录下的los_mip_mem.c和los_mip_osfunc.c中的外部接口全部实现。

- los_mip_mem.c主要实现的是内存池的初始化、内存的申请释放等接口，如果系统可以直接malloc，那么可以将该函数实现为空函数，内存的malloc和释放直接使用系统函数即可。

- los_mip_osfunc.c 主要实现了任务创建，销毁、消息队列的创建、读写、信号量互斥锁等进程通讯接口的实现。
在移植LosMIP时，必须将相关的接口替换成新的操作系统下的接口。






		