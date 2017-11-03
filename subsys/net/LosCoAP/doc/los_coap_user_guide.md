## 关于LosCoAP

   LosCoAP 是为华为LiteOS开发的一个CoAP Client端的协议栈，并且集成了与华为IoT平台对接的CoAP客户端AgentTiny。


## LosCoA的目录结构简介

doc --> LosCoAP的文档

include --> LosCoAP的所有头文件

src --> LosCoAP协议实现的C代码目录

test --> 测试代码存放目录


## 如何编译测试

### 一： 测试前的环境准备

在测试LosCoAP之前需要准备测试用的coap服务器。

- 操作系统：ubuntu12.04或者以上版本

- coap服务器程序：libcoap

如何编译使用libcoap请参考如下链接内容

https://libcoap.net/doc/install.html#code

- 其他说明：


编译完毕libcoap之后会在其目录下的examples目录下生成coap-server可以执行程序
在ubuntu下直接通过命令启动该server，到此为止测试环境搭建完成。



### 二： linux下测试LosCoAP程序

- 修改main.c中的coap测试服务器的地址
remoteser = los_coap_new_resource("192.168.206.41", 5683);为utuntu真实的IP地址。比如修改为192.168.1.100

- 进入LosCoAP执行make


- 编译完成后会生成一个coap_client可执行程序，通过./coap_client执行客户端
  可以看到printf输出接收的数据包的内容（测试程序只请求了一个包）。


### 三： 嵌入式环境如何使用LosCoAP

- 目前LosCoAP已经在LiteOS系统上进行了相关的适配，并且网络协议栈使用的LWIP。
  移植LosCoAP到LiteOS上，需要将src目录下的coap_core.c、coap_os_liteos.c加入到工程中编译，

- 将test目录下的los_coap_test.c加入LiteOS工程进行编译。

- 在LiteOS的main.c中加入coap_test_init()调用。

- 修改coaptest_task()函数中的coap服务器的地址为测试的coap服务器地址。remoteser = los_coap_new_resource("192.168.206.41", 5683);比如，服务器的地址为192.168.1.100，则修改为los_coap_new_resource("192.168.1.100", 5683);


## LosCoAP接口说明

- int los_coap_stack_init(void);

  初始化coap协议栈

- void *los_coap_new_resource(char *ipaddr, unsigned short port);

  设置coap客户端连接的服务器的ip地址和端口，并保存在void *类型的指针指向的地址。

- coap_context_t *los_coap_malloc_context(void *res);

  创建coap客户端instance。函数参数为los_coap_new_resource返回值。

- coap_option_t * los_coap_add_option_to_list(coap_option_t *head,unsigned short option,char *value, int len);

  创建coap消息的option列表，函数返回值是option列表的头结点。head参数是option的头结点，剩余参数是option对应的内容


- coap_msg_t *los_coap_new_msg(coap_context_t *ctx,
								unsigned char msgtype, 
								unsigned char code, 
								coap_option_t *optlist, 
								unsigned char *paylaod, 
								int payloadlen);

  创建要发送的coap消息。

- int los_coap_add_option(coap_msg_t *msg, coap_option_t *opts);

  将option列表添加到coap消息中

- int los_coap_generate_token(unsigned char *token);

  生成随机token，并返回token的长度

- int los_coap_add_token(coap_msg_t *msg, char *tok, int tklen);

  将token添加到coap消息中

- int los_coap_add_paylaod(coap_msg_t *msg, char *payload, int len);

  添加coap消息的payload数据。

- int los_coap_register_handler(coap_context_t *ctx, msghandler func);

  注册收到coap服务器发送回来的coap消息的回调处理函数


- int los_coap_add_resource(coap_context_t *ctx, coap_res_t *res); 

  添加coap客户端的本地资源到coap客户端instance中，如果coap服务器通过消息获取coap客户端的资源，所有对资源的访问都将通过res中的回调函数处理。

- int los_coap_read(coap_context_t *ctx);

  收取coap服务器发送来的消息，并进行相关内部处理。

- int los_coap_send(coap_context_t *ctx, coap_msg_t *msg);

  发送coap消息给服务器。

