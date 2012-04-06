/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-03-27 15:29
# Filename: server.c
# Description: 
============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <iostream>
#include <err.h>
#include <event.h>
#include <evhttp.h>

#include "leveldb/db.h" 
#include "leveldb/comparator.h"
#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "server.h"

int settings_cache = 100;
char settings_dataname[1024];
char* pidfile = "/tmp/server.pid";

void show_help()
{
	fprintf(stdout, "this is show help()\n");
	//XXX
}

static void kill_signal_worker(const int sig)
{
	delete db;
	exit(0);
}

static void kill_signal_master(const int sig)
{
	remove(pidfile);
	kill(0, SIGTERM);
	exit(0);
}

typedef void(*ptr_event)(const int sig);

void kill_signal_register(ptr_event p)
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, p);
	signal(SIGKILL, p);
	signal(SIGQUIT, p);
	signal(SIGTERM, p);
	signal(SIGHUP, p);
}

int httpserver_init(char *listen, int port, int timeout)
{
	struct evhttp *httpd;
	event_init();
	httpd = evhttp_start(listen, port);
	if (httpd == NULL)
	{
		fprintf(stderr, "Error: Unable to listen on %s:%d\n\n", listen, port);
		kill(0, SIGTERM);
		exit(-1);
	}
	evhttp_set_timeout(httpd, timeout);
	evhttp_set_gencb(httpd, http_handler, NULL);
	event_dispatch();
	evhttp_free(httpd);
	return 0;
}

int main(int argc, char *argv[])
{
	char *listen = "0.0.0.0";
	int port = 12345;
	int daemon = false;
	int timeout = 60;
	char *datapath;
	int c;

	while ((c = getopt(argc, argv, "l:p:x:t:c:m:i:a:dh")) != -1)
	{
		switch (c)
		{
			case 'l':
				listen = strdup(optarg);
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'x':
				datapath = strdup(optarg);
				if (access(datapath, W_OK) != 0)
				{
					if (access(datapath, R_OK) == 0)
						chmod(datapath, S_IWOTH);
					else 
						create_multilayer_dir(datapath);

					if (access(datapath, W_OK) != 0)
						fprintf(stderr, "database directory not writeable!\n");
				}
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'c':
				settings_cache = atoi(optarg) > 0 ? atoi(optarg) : 100;
				break;
			case 'i':
				pidfile = strdup(optarg);
				break;
			case 'd':
				daemon = true;
				break;
			case 'h':
			default:
				show_help();
				return -1;
		}
	}

	if (datapath == NULL)
	{
		show_help();
		fprintf(stderr, "Attention: Please use the indispensable argument: -x <path>\n\n");
		return -1;
	}
	
	sprintf(settings_dataname, "%s/server.db", datapath);

	if (!opendb().ok())
	{
		show_help();
		fprintf(stderr, "Attention: database open failed.\n\n");
		return -1;
	}

	if (daemon)
	{
		pid_t pid = fork();	//fork off the parent process
		if (pid < 0)
			return -1;
		if (pid > 0)		//exit parent process
			return 0;
	}

	FILE *fp_pidfile = fopen(pidfile, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);

	pid_t worker_process_pid = fork();

	if (worker_process_pid < 0)
	{
		fprintf(stderr, "Error: %s:%d\n", __FILE__, __LINE__);
		return -1;
	}
	if (worker_process_pid > 0)	//parent process
	{
		kill_signal_register(&kill_signal_master);
		while (1)
		{
			if (wait(NULL) < 0)
				continue;
			usleep(100000);
			worker_process_pid = fork();
			if (worker_process_pid == 0)
				break;
		}
	}

	kill_singal_register(&kill_signal_worker);

	httpserver_init(listen, port, timeout);

	return 0;
}
