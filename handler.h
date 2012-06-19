/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:14
# Filename: handler.h
# Description: 
============================================================================*/
#ifndef FULLFS_HANDLER
#define FULLFS_HANDLER

int httpserver_init(char *listen, int port, int timeout);
void http_handler(struct evhttp_request *req, void *arg);

#endif
