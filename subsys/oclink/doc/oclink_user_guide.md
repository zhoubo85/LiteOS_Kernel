## oclink简介
- oclink是huawei LiteOS上的一个连接华为云平台ocean connect的接口集合。通过调用oclink的接口，用户可以很方便的将自己的设备与华为云平台连接起来，将设备的数据上传到华为云平台，并且用户也可以根据设备的特性通过云平台给设备发送数据或者命令。

## oclink依赖
- oclink是基于LosCoAP进行开发的，所以要使用oclink必须加入LosCoAP，否则无法使用oclink的相关功能，并且编译也会存在问题。

## oclink接口介绍
- int los_oclink_set_param(oclink_param_t kind, void *data, int len)
  
  该接口是用于设置oclink的参数的接口。主要有如下几类参数需要设置：

	OCLINK_IOTS_ADDR  设置设备连接的华为云平台的ip地址，参数是字符串类型

	OCLINK_IOTS_PORT  设置待设备连接的华为云平台的的端口号，参数是unsigned short指针

	OCLINK_IOT_DEVSN  设置设备的唯一标识号，参数是字符串类型

	OCLINK_CMD_CALLBACK 设置设备处理服务器反馈数据或者命令的回调函数，参数是函数指针

- int los_oclink_start(void)
  
	启动oclink内部任务。该接口在正确设置参数之后才调用。

- int los_oclink_stop(void)

	停止oclink内部任务，是否oclink申请的相关资源。

- int los_oclink_send(char *buf, int len)

	该接口是oclink上报数据给华为云平台的接口。其中的buf的内容是需要根据云平台的编解码插件和设备profile。

## oclink接口使用示例
演示demo的主逻辑如下

	int ret = 0;
	int len = 0
	unsigned char data[8] = {0};
	unsigned short iotports = 5683;

	/* 设置设备要连接的云平台的ip地址 */
	ret = los_oclink_set_param(OCLINK_IOTS_ADDR, "10.1.12.5", strlen("10.1.12.5"));
	/* 设置设备要连接的云平台的端口 */
	ret = los_oclink_set_param(OCLINK_IOTS_PORT, &iotports, 0);
	/* 设置设备的唯一序列号（即设备的verifycdoe） */
	ret = los_oclink_set_param(OCLINK_IOT_DEVSN, "liteos000002", strlen("liteos000002"));

	/* 如果设置参数得返回值小于0，则说明参数设置失败，需要检查参数传递是否正确 */

	/* 设置设备处理平台下发数据或者命令的回调函数 */
	ret = los_oclink_set_param(OCLINK_CMD_CALLBACK, (void *)oclink_recv_cmd, 0);
	/* 启动oclink内部任务 */
	ret = los_oclink_start();
	if (ret < 0)
	{
		/* 启动oclink内部任务失败，需要销毁相关资源，或者用户使用完毕也可以销毁资源 */
		ret = los_oclink_stop();
		return ;
	}
	/* 上传本地数据给华为云平台 */
	los_oclink_send((char *)data, len);

云平台反馈数据或命令的回调函数

	void oclink_recv_cmd(unsigned char *pucval, int nlen)
	{
		/* 
			if the data is expected data , something todo
			for example: we expect  pucval[0] == 0x01 or pucval[0] == 0x02
		*/
		if(pucval[0] == 0x01 || pucval[0] == 0x02)
		{
			/* something todo */
			//.....
		}
	}


## 完整的参考demo

- oclink中提供了一个完整的用于演示oclink的demo，实现在los_oclink_demo.c中。


