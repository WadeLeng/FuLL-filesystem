#ifndef KVFS_SERVER
#define KVFS_SERVER

#include "config.h"
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
#include "utils.h"

typedef struct iteminfo 
{
	char* key;
	char* value;
	int value_size;
	time_t exptime;
}iteminfo;//XXX

static leveldb::DB* db;
static leveldb::Options options;
leveldb::Status opendb();
leveldb::Status put(const iteminfo item);
const iteminfo get(const char* key);
leveldb::Status clear(const char* key);
leveldb::Status clearall();
void http_handler(struct evhttp_request *req, void *arg);

#endif
