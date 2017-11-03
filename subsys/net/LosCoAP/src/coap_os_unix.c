#include "../include/los_coap.h"
#include "../include/coap_core.h"

//#define WITH_UNIX

#ifdef WITH_UNIX
#include<sys/types.h> 
#include<sys/socket.h> 
#include<unistd.h> 
#include<netinet/in.h> 
#include<arpa/inet.h> 
#include<stdio.h> 
#include<stdlib.h> 
#include<errno.h> 
#include<netdb.h> 
#include<stdarg.h> 
#include<string.h> 
#include <time.h>

#include "../include/los_unix.h"


#define G_LOS_COAP_SMALL_STRUCT_MAX 64

unsigned char g_los_token[4] = {0};




int los_coap_unix_send(void *handle, char *buf, int size);
int los_coap_unix_read(void *handle, char *buf, int size);

struct udp_ops network_ops = 
{
    .network_read = los_coap_unix_read,
    .network_send = los_coap_unix_send
};

static unsigned int los_coap_get_random(void)
{
	unsigned int ret;
    time_t t;
    srand((unsigned) time(&t));
	ret = rand() % 0x7fffffff;
	return ret;
}

int los_coap_generate_token(unsigned char *token)
{
	unsigned int ret;
    time_t t;
    srand((unsigned) time(&t));
	ret = rand() % RAND_MAX;
    token[0] = (unsigned char)ret;
    token[1] = (unsigned char)(ret>>8);
    token[2] = (unsigned char)(ret>>16);
    token[3] = (unsigned char)(ret>>24);
	return 4;
}


int los_coap_stack_init(void)
{   
 	return 0;   
}

void *los_coap_malloc(int size)
{
    char *tmp = NULL;
    tmp = (char *)malloc(size);
    return (void *)tmp;
}

int los_coap_free(void *p)
{
    if (NULL == p)
	{
		return -1;
	}

    free(p);
    return 0;
}

static int los_coap_check_validip(char *ipaddr)
{
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int ret = 0;
    if(NULL == ipaddr)
    {
        return -1;
    }
    ret = sscanf(ipaddr, "%d.%d.%d.%d", &a,&b,&c,&d);
    if (ret != 4)
    {
        return -1;
    }
    if ((a >= 0 && a <= 255)
        && (b >=0 && b <= 255)
        && (c >=0 && c <= 255)
        && (d >=0 && d <= 255))
    {
        return 0;
    }
    return -1;
}
void *los_coap_new_resource(char *ipaddr, unsigned short port)
{
    int ret = 0;
    struct unix_udp_res_t *tmp = NULL;
    if (NULL == ipaddr)
    {
        return NULL;
    }
    ret = los_coap_check_validip(ipaddr);
    if (ret < 0)
    {
        return NULL;
    }
    tmp = (struct unix_udp_res_t *)malloc(sizeof(struct unix_udp_res_t));
    if (NULL == tmp)
    {
        return NULL;
    }
    memset(tmp, 0, sizeof(struct unix_udp_res_t));
    tmp->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tmp->fd < 0)
    {
        free(tmp);
        return NULL;
    }
    tmp->remoteAddr.sin_addr.s_addr = inet_addr((const char *)ipaddr);
    tmp->remoteAddr.sin_port = htons(port);
    tmp->remoteAddr.sin_family = AF_INET;
    return (void *)tmp;
}

static int los_coap_delete_resource(void *res)
{
    struct unix_udp_res_t *tmp = NULL;
    if (NULL == res)
	{
		return -1;
	}
    tmp = (struct unix_udp_res_t *)res;
    if (tmp->fd >= 0)
    {
        close(tmp->fd);
    }
    free(tmp);
    return 0;
}

coap_context_t *los_coap_malloc_context(void *res)
{
	coap_context_t *tmp = NULL;
	if (NULL == res)
	{
		return NULL;
	}
	
	tmp = (coap_context_t *)malloc(sizeof(coap_context_t));
	if (NULL == tmp)
	{
		return NULL;
	}
	
	// FIXED ME 
	tmp->udpio = res;
	
	tmp->sndbuf.buf = (unsigned char *)malloc(LOSCOAP_CONSTRAINED_BUF_SIZE);
	if (NULL == tmp->sndbuf.buf)
	{
        free(tmp);
		return NULL;
	}
	tmp->sndbuf.len = LOSCOAP_CONSTRAINED_BUF_SIZE;
	
	tmp->rcvbuf.buf = (unsigned char *)malloc(LOSCOAP_CONSTRAINED_BUF_SIZE);
	if (NULL == tmp->rcvbuf.buf)
	{
        free(tmp);
        free(tmp->sndbuf.buf);
		return NULL;
	}
    
	tmp->rcvbuf.len = LOSCOAP_CONSTRAINED_BUF_SIZE;
	tmp->msgid = (unsigned short)los_coap_get_random();
    tmp->netops = &network_ops;
    
    tmp->response_handler = NULL;
	return tmp;	
}

int los_coap_free_context(coap_context_t *ctx)
{
	coap_context_t *tmp = ctx;
    
	if (NULL == ctx)
	{
		return -1;
	}
	if (NULL != tmp->udpio)
    {
        los_coap_delete_resource(tmp->udpio);
        tmp->udpio = NULL;
    }

    if (NULL != tmp->sndbuf.buf)
    {
        free(tmp->sndbuf.buf);
        tmp->sndbuf.buf = NULL;
        tmp->sndbuf.len = 0;
    }
    if (NULL != tmp->rcvbuf.buf)
    {
        free(tmp->rcvbuf.buf);
        tmp->rcvbuf.buf = NULL;
        tmp->rcvbuf.len = 0;
    }
    tmp->netops = NULL;
    tmp->response_handler = NULL;
    free(ctx);
    
	return 0;	
}

int los_coap_unix_send(void *handle, char *buf, int size)
{
    int n = 0;
    struct unix_udp_res_t *res = NULL;
    if (NULL == handle || NULL == buf)
    {
        return -1;
    }
    res = (struct unix_udp_res_t *)handle;
    if (res->fd < 0)
    {
        return -1;
    }
	n = sendto(res->fd,
			buf,
			size,
			0, 
			(struct sockaddr *)&res->remoteAddr,
			sizeof(struct sockaddr_in));
    return n;
}

int los_coap_unix_read(void *handle, char *buf, int size)
{
    int n = 0;
    struct sockaddr_in fromAddr;
    socklen_t fromLen;

    struct unix_udp_res_t *res = NULL;
    if (NULL == handle || NULL == buf)
    {
    	printf("read error param\n");
        return -1;
    }
    res = (struct unix_udp_res_t *)handle;
    if (res->fd < 0)
    {
    	printf("socket error handle\n");
        return -1;
    }

    bzero(&fromAddr,sizeof(fromAddr));  
    fromAddr.sin_family = AF_INET;  
    fromAddr.sin_addr.s_addr = htonl(INADDR_ANY);  
    fromAddr.sin_port = res->remoteAddr.sin_port;  
    fromLen=sizeof(fromAddr);
    
    n = recvfrom( res->fd, 
                buf, size, 
                0, 
                (struct sockaddr *)&fromAddr, 
                &fromLen );
    return n;
}


#endif /* WITH_UNIX */

