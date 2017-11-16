## LosCoAP简介
- LosCoAP是专门为华为的LiteOS系统上开发的一个CoAP的client端的CoAP版本。LosCoAP是为oclink服务的，其适用于ram比较小的嵌入式系统中，比如ram只有几十K的嵌入式芯片。

## LosCoAP代码简介
- src：该目录存放coap协议解析的相关代码
- include：该目录存放coap协议实现中需要的头文件
- test：该目录存放了测试coap的测试代码。

- 目前LosCoAP还有许多特性没有加入，比如如下的一些特性：

    1:不支持dtls

    2:不支持proxying

    3:不支持multicast coap

    4:也不支持完全解析所有的option，只解析比较通用的option。


## LosCoAP使用入门

在LiteOS上使用LosCoAP开发coap客户端程序，请参考如下的使用步骤

- 初始化LosCoAP协议栈，调用如下接口：

		los_coap_stack_init(); 

- 创建CoAP的的实例，我们简称ctx。

		remoteser = los_coap_new_resource("192.168.206.41", 5683);
		ctx = los_coap_malloc_context(remoteser);

- 创建coap消息

		opts = los_coap_add_option_to_list(opts,COAP_OPTION_URI_PATH,".well-known", 11);
		//.....
		opts = los_coap_add_option_to_list(opts,COAP_OPTION_BLOCK2,(char *)&blockdata, 1);
		msg = los_coap_new_msg(ctx,
							COAP_MESSAGE_CON, 
							COAP_REQUEST_GET, 
							opts, 
							paylaod, 
							payloadlen);
- 注册服务器反馈消息处理函数

		ret = los_coap_register_handler(ctx, handle_coap_response);

- 发送消息

		ret = los_coap_send(ctx, msg);

- 接收消息

		ret = los_coap_read(ctx);

完整的示例，请参考test目录下的los_coap_test.c


## LosCoAP移植指导
- 如果想将LosCoAP移植到不同的系统上面，必须实现以下内容：

		/* 内存池初始化以及其他所依赖的操作系统的资源的初始化 */
		int los_coap_stack_init(void)

		/* 完成 network_ops 结构体的实现，根据实际平台的socket读写接口完成 */
		int los_coap_unix_send(void *handle, char *buf, int size);
		int los_coap_unix_read(void *handle, char *buf, int size);
		struct udp_ops network_ops = 
		{
		    .network_read = los_coap_unix_read,
			.network_send = los_coap_unix_send
		};
		
		/* 完成radom接口的实现 */
		static unsigned int los_coap_get_random(void)

		/* 完成随机数token的生成函数 */
		int los_coap_generate_token(unsigned char *token)

		/* 完成malloc和free接口的适配 */
		void *los_coap_malloc(int size)
		int los_coap_free(void *p)

		/* 完成socket资源创建销毁函数的适配 */
		void *los_coap_new_resource(char *ipaddr, unsigned short port)
		static int los_coap_delete_resource(void *res)

		/* 完成coap context的创建销毁函数 */
		static int los_coap_delete_resource(void *res)
		int los_coap_free_context(coap_context_t *ctx)
		
		/* 完成socket读写函数的适配 */
		int los_coap_unix_send(void *handle, char *buf, int size)
		int los_coap_unix_read(void *handle, char *buf, int size)

- 移植的中的所有函数的示例可以参考coap_os_liteos.c或者coap_os_unix.c






