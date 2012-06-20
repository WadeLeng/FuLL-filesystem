/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:14
# Filename: handler.c
# Description: httpserver_init启动libevent, http_handler处理请求函数
============================================================================*/
#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <iostream>

#include <err.h>
#include <event.h>
#include <evhttp.h>


#include "database.h"

/* 事件处理函数 */
void http_handler(struct evhttp_request *req, void *arg)
{
	/* 只处理GET POST */
	if (EVHTTP_REQ_GET != req->type && EVHTTP_REQ_POST != req->type)
	{
		evhttp_send_error(req, 502, "Bad Gateway");
		return;
	}

	struct evbuffer *buf = evbuffer_new();

	/* 分析URL参数 */
	char *decode_uri = strdup((char*) evhttp_request_uri(req));
	struct evkeyvalq http_query;
	evhttp_parse_query(decode_uri, &http_query);
	free(decode_uri);

	/* 接收GET表单参数 */
	const char *opt = evhttp_find_header(&http_query, "opt");
	const char *key = evhttp_find_header(&http_query, "key");
	const char *value = evhttp_find_header(&http_query, "value");

	/* 处理输出header头 */
	evhttp_add_header(req->output_headers, "Content-Type", "text/plain");
	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	evhttp_add_header(req->output_headers, "Connection", "close");

	/* 解析参数 */
	if (opt)
	{
		if (strcmp(opt, "put") == 0 && key != NULL)
		{
			/* 优先接收POST正文信息 */
			int buffer_data_len = EVBUFFER_LENGTH(req->input_buffer);
			if (buffer_data_len > 0)
			{
				char *buffer_data = (char*) calloc(buffer_data_len, sizeof(char));
				memcpy(buffer_data, EVBUFFER_DATA(req->input_buffer), buffer_data_len);
				/* put 到leveldb 中 */
				put(key, buffer_data, strlen(buffer_data));
				evhttp_add_header(req->output_headers, "Key", key);
				evbuffer_add_printf(buf, "%s", "SERVER_SET_OK");
				free(buffer_data);
			}
			/* 如果POST正文无内容，则取URL中data参数的值 */
			else if (value != NULL)
			{
				if (strlen(value) > 0)
				{
					put(key, value, strlen(value));
					evhttp_add_header(req->output_headers, "Key", key);
					evbuffer_add_printf(buf, "%s", "SERVER_PUT_END");
				}
				else
					evbuffer_add_printf(buf, "%s", "SERVER_PUT_END");
			}
			else
				evbuffer_add_printf(buf, "%s", "SERVER_PUT_ERROR");
		}
		else if (strcmp(opt, "get") == 0 && key != NULL)
		{
			char *buf_value = NULL;
			int i, length = 0;
			if (get(key, &buf_value, &length).ok())
			{
				evhttp_add_header(req->output_headers, "Key", key);
				evbuffer_add_printf(buf, "%s", buf_value);
				free(buf_value);
			}
			else
			{
				evbuffer_add_printf(buf, "%s", "SERVER_GET_ERROR");
			}
		}
		else if (strcmp(opt, "delete") == 0 && key != NULL)
		{
			if (!clear(key).ok())
				evbuffer_add_printf(buf, "%s", "SERVER_DELETE_ERROR");
			else
				evbuffer_add_printf(buf, "%s", "SERVER_DELETE_OK");
		}
		else
		{
			evbuffer_add_printf(buf, "%s", "SERVER_OPT_ERROR");
		}
	}
	else
		evbuffer_add_printf(buf, "%s", "SERVER_OPT_ERROR");
	/* 返回code 200 */
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	/* 释放内存 */
	evhttp_clear_headers(&http_query);
	evbuffer_free(buf);
}

int httpserver_init(char *listen, int port, int timeout)
{
	struct evhttp *httpd;
	/* 初始化监听IP和端口 */
	event_init();
	httpd = evhttp_start(listen, port);
	if (httpd == NULL)
	{
		fprintf(stderr, "Error: Unable to listen on %s:%d\n\n", listen, port);
		/* 给进程组所有进程发送终止信号 */
		kill(0, SIGTERM);
		exit(-1);
	}
	/* 设置HTTP连接超时时间 */
	evhttp_set_timeout(httpd, timeout);
	/* 设置请求到达后的回调函数 */
	evhttp_set_gencb(httpd, http_handler, NULL);
	/* libevnet 循环处理事件 */
	event_dispatch();
	/* 释放资源 */
	evhttp_free(httpd);
	return 0;
}
