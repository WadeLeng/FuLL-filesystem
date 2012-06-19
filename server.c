/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:29
# Filename: server.c
# Description: 
============================================================================*/
#define FUSE_USE_VERSION 26

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
#include <fuse.h>

#include "leveldb/iterator.h"
#include "leveldb/db.h" 
#include "leveldb/comparator.h"
#include "leveldb/write_batch.h"
#include "leveldb/cache.h"

#include "database.h"
#include "utils.h"
#include "handler.h"
#include "server.h"

using namespace std;

char* pidfile = (char*) "/tmp/server.pid";
int server_settings_cache = 100;
char server_settings_dataname[1024];
leveldb::DB* db;
leveldb::Options full_options;

static int full_fs_getattr(const char* path, struct stat* stbuf)
{
	int res = 0, value_size;
	char* key = strdup(path), *value = NULL;
	leveldb::Status s;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
	} else 
	{
		stbuf->st_mode = S_IFREG | 0444;
		s = get(key + 1, &value, &value_size);
		if (!s.ok())
			res = -ENOENT;
		else
			stbuf->st_size = value_size;
		free(value);
	}

	free(key);
	return res;
}

static int full_fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
				off_t offset, struct fuse_file_info *fi)
{
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	char *key;
	int key_size = 0;

	if (strcmp(path, "/") != 0)
		return -ENOENT;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		key_size = it->key().ToString().length();
		key = (char*) malloc(key_size * sizeof(char) + 1);

		strcpy(key, it->key().ToString().c_str());
		filler(buf, key, NULL, 0);
		free(key);
	}
	delete it;

	return 0;	
}

static int full_fs_mkdir(const char* path, mode_t mode)
{
	return -EPERM;	//no permission, only root dir:/
}

static int full_fs_rmdir(const char* path)
{
	return -EPERM;
}

static int full_fs_mknod(const char* path, mode_t mode, dev_t rdev)
{
	return -EPERM;	// full_fs RDONLY
}

static int full_fs_read(const char* path, char* buf, size_t size, off_t offset,
				struct fuse_file_info* fi)
{
	int value_size;
	size_t len;
	char* key = strdup(path);
	char* value;
	leveldb::Status s;

	s = get(key + 1, &value, &value_size);
	if (!s.ok())
	{
		free(value);
		return -ENOENT;
	}

	len = (size_t) value_size;
	if (offset < len)
	{
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, value + offset, size);
	} else
		size = 0;

	free(value);
	free(key);
	return size;
}

static int full_fs_write(const char* path, const char* buf, size_t size,
				off_t offset, struct fuse_file_info* fi)
{
	return -ENOENT;
}

static int full_fs_unlink(const char* path)
{
	return -EPERM;
}

static int full_fs_open(const char* path, struct fuse_file_info* fi)
{
	int res = 0, value_size;
	char* key = strdup(path), *value;
	leveldb::Status s;

	if (strcmp(path, "/") == 0)
		res = 0;
	else
	{
		s = get(key + 1, &value, &value_size);
		if (!s.ok())
			res = -ENOENT;
		else
			res = 0;
		free(value);
	}

	free(key);
	return res;
}

static int full_fs_flush(const char* path, struct fuse_file_info* fi)
{
	return 0;
}

static int full_fs_truncate(const char* path, off_t size)
{
	return 0;
}

static struct full_fs_operations : fuse_operations
{
	full_fs_operations()
	{
		getattr	= full_fs_getattr;
		readdir	= full_fs_readdir;
		mkdir	= full_fs_mkdir;
		rmdir	= full_fs_rmdir;
		mknod	= full_fs_mknod;
		read	= full_fs_read;
		write	= full_fs_write;
		unlink	= full_fs_unlink;
		open	= full_fs_open;
		flush	= full_fs_flush;
		truncate	= full_fs_truncate;
	}
} full_fs_oper;

void* fuse_init(void* f_argv)
{
	fuse_main(3, (char**)f_argv, &full_fs_oper, NULL);
	pthread_exit(NULL);
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

int main(int argc, char *argv[])
{
	char* listen = (char*) "0.0.0.0";
	char* f_argv[4];
	int port = 12345;
	int daemon = false;
	int timeout = 60;
	char *datapath;
	int c;

	while ((c = getopt(argc, argv, "l:p:x:t:c:m:i:a:r:dh")) != -1)
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
				if (access(datapath, W_OK) != 0)	//test write
				{
					if (access(datapath, R_OK) == 0) //test read
						chmod(datapath, S_IWOTH);	//others write
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
				server_settings_cache = atoi(optarg) > 0 ? atoi(optarg) : 100;
				break;
			case 'i':
				pidfile = strdup(optarg);
				break;
			case 'r':
				f_argv[0] = argv[0];
				f_argv[1] = strdup(optarg);
				f_argv[2] = (char*)"-f";
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
	
	sprintf(server_settings_dataname, "%s/server.db", datapath);

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

	kill_signal_register(&kill_signal_worker);

	pthread_t pid;
	pthread_create(&pid, NULL, fuse_init, f_argv);

	httpserver_init(listen, port, timeout);

	pthread_join(pid, NULL);

	return 0;
}
