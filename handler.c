#include <err.h>
#include <event.h>
#include <evhttp.h>

#include "server.h"

void http_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *buf = evbuffer_new();

	/* 分析URL参数 */
	char *decode_uri = strdup((char*) evhttp_request_uri(req));
	struct evkeyvalq http_query;
	evhttp_parse_query(decode_uri, &http_query);
	free(decode_uri);

	/* 接收GET表单参数 */
	const char *opt = evhttp_find_header(&http_query, "opt");
	const char *key = evhttp_find_header(&http_query, "key");
	const char *input_value = evhttp_find_header(&http_query, "value");
	const char *exptime = evhttp_find_header(&http_query, "exptime");

	/* 处理输出header头 */
	evhttp_add_header(req->output_headers, "Content-Type", "text/plain");
	evhttp_add_header(req->output_headers, "Connection", "keep-alive");
	evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");

	/* 解析参数 */
	if (opt)
	{
		if (exptime == NULL) exptime = "0";

		if (strcmp(opt, "put") == 0 && key != NULL)
		{
		}
		else if (strcmp(opt, "get") == 0 && key != NULL)
		{
		}
		else if (strcmp(opt, "delete") == 0 && key != NULL)
		{
		}
		else
		{
			evbuffer_add_printf(buf, "%s", "Error: opt norecognition");
		}
	}
	else
	{
		evbuffer_add_printf(buf, "%s", "Error: no opt");
	}
	/* 返回code 200 */
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	/* 释放内存 */
	evhttp_clear_headers(&http_query);
	evbuffer_free(buf);
}
