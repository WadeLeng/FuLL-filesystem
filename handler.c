#include <err.h>
#include <event.h>
#include <evhttp.h>

#include "server.h"

void http_handler(struct evhttp_request *req, void *arg)
{
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
	//const char *exptime = evhttp_find_header(&http_query, "exptime");

	/* 处理输出header头 */
	evhttp_add_header(req->output_headers, "Content-Type", "text/plain");
	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");
	evhttp_add_header(req->output_headers, "Connection", "close");

	/* 解析参数 */
	if (opt)
	{
		if (exptime == NULL) exptime = "0";

		if (strcmp(opt, "put") == 0 && key != NULL)
		{
			/* 优先接收POST正文信息 */
			int buffer_data_len = EVBUFFER_LENGTH(req->input_buffer);
			if (buffer_data_len > 0)
			{
				char *buffer_data = (char*) calloc(buffer_data_len, sizeof(char));
				memcpy(buffer_data, EVBUFFER_DATA(req->input_buffer), buffer_data_len);
				put(key, buffer_data, strlen(buffer_data));//FIXME: \0
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
			char *value = NULL;
			int i, length = 0;
			if (get(key, value, &length).ok())
			{
				evhttp_add_header(req->output_headers, "Key", key);
				for (i = 0; i < length; i++)
					evbuffer_add_printf(buf, "%c", value[i]);
				//free(value);
			}
			else
				evbuffer_add_printf(buf, "%s", "SERVER_GET_ERROR");
		}
		else if (strcmp(opt, "delete") == 0 && key != NULL)
		{
			if (!clear(key).ok())
				evbuffer_add_printf(buf, "%s", "SERVER_DELETE_ERROR");
			else
				evbuffer_add_printf(buf, "%s", "SERVER_DELETE_OK");
		}
		else if (strcmp(opt, "deleteall") == 0)
		{
			if (!clearall().ok())
				evbuffer_add_printf(buf, "%s", "SERVER_DELETE_ALL_ERROR");
			else
				evbuffer_add_printf(buf, "%s", "SERVER_DELETE_ALL_OK");
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
