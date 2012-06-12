#ifndef FULLFS_SERVER
#define FULLFS_SERVER

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

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "database.h"

extern int server_settings_cache;
extern char server_settings_dataname[1024];
static leveldb::DB* db;
static leveldb::Options options;

leveldb::Status opendb();
leveldb::Status put(const char* key, const char* value, int value_size);
leveldb::Status get(const char* key, char** value, int* value_size);
leveldb::Status clear(const char* key);
void http_handler(struct evhttp_request *req, void *arg);

#endif
