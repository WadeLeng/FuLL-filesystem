/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:29
# Filename: server.c
# Description: 实现FUSE接口，及主线程
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

//存放pid的文件
char* pidfile = (char*) "/tmp/full_fs.pid";
//leveldb缓存，默认100MB
int server_settings_cache = 100;
//leveldb路径
char server_settings_dataname[1024];
leveldb::DB* db;
leveldb::Options full_options;

/*
 * 查看path是路径还是文件，并赋权限。
 * 文件都是只读的。
 * 如果不存在返回-ENOENT
 * 成功返回0
 */
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

/*
 * 列出根目录下所有文件名
 * 如果不在根目录下，返回 -ENOENT
 * 成功返回0
 */
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

/*
 * 只读文件系统，不能创建目录
 * 直接返回 -EPERM
 */
static int full_fs_mkdir(const char* path, mode_t mode)
{
	return -EPERM;	//no permission
}

/*
 * 只读文件系统，不能删除目录
 * 直接返回 -EPERM
 */
static int full_fs_rmdir(const char* path)
{
	return -EPERM;
}

/*
 * 只读文件系统，不能创建文件
 * 直接返回 -EPERM
 */
static int full_fs_mknod(const char* path, mode_t mode, dev_t rdev)
{
	return -EPERM;	// full_fs RDONLY
}

/*
 * 根据path从leveldb里读数据，到buf里。从offset处开始
 * 如果存在文件，返回size
 * 否则返回 -ENOENT
 */
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

/*
 * 只读文件系统，不可写
 * 直接返回 -EPERM
 * 只有libevent端可以写数据
 */
static int full_fs_write(const char* path, const char* buf, size_t size,
				off_t offset, struct fuse_file_info* fi)
{
	return -EPERM;
}

/*
 * 没有权限
 * 直接返回 -EPERM
 */
static int full_fs_unlink(const char* path)
{
	return -EPERM;
}

/*
 * 根据path看文件是否存在
 * 如果不存在返回 -ENOENT
 * 成功返回0
 */
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

/*
 * This function should not be modified
 */
static int full_fs_flush(const char* path, struct fuse_file_info* fi)
{
	return 0;
}

/*
 * This function should not be modified
 */
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

/* FUSE线程 */
void* fuse_init(void* f_argv)
{
	int f_argc = 3;

	fuse_main(f_argc, (char**)f_argv, &full_fs_oper, NULL);
	pthread_exit(NULL);
}

/* 子进程信号处理 */
static void kill_signal_worker(const int sig)
{
	delete db;
	exit(0);
}

/* 父进程信号处理 */
static void kill_signal_master(const int sig)
{
	remove(pidfile);
	/* 给进程组所有进程发送终止信号 */
	kill(0, SIGTERM);
	exit(0);
}

typedef void(*ptr_event)(const int sig);

/* 处理kill信号 */
void kill_signal_register(ptr_event p)
{
	signal(SIGINT, p);
	signal(SIGKILL, p);
	signal(SIGQUIT, p);
	signal(SIGTERM, p);
	signal(SIGHUP, p);
}

int main(int argc, char *argv[])
{
	/* 绑定所有IP */
	char* listen = (char*) "0.0.0.0"; 
	/* fuse启动参数 */
	char* f_argv[4] = {NULL};
	/* 端口号 */
	int port = 12345;
	/* 后台运行 */
	int daemon = false;
	/* http请求超时时间 */
	int timeout = 60;
	/* leveldb路径 */
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
				/* 测试是否可写 */
				if (access(datapath, W_OK) != 0)
				{
					/* 测试是否可读 */
					if (access(datapath, R_OK) == 0)
						/*others可写*/
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
				server_settings_cache = atoi(optarg) > 0 ? atoi(optarg) : 100;
				break;
			case 'i':
				pidfile = strdup(optarg);
				break;
			case 'r':
				f_argv[0] = argv[0];
				f_argv[1] = strdup(optarg);
				/* 非后台运行，否则无法启动libevnet进程就关闭了 */
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

	/* 必填参数 数据库路径 */
	if (datapath == NULL)
	{
		show_help();
		fprintf(stderr, "Attention: Please use the indispensable argument: -x <path>\n\n");
		return -1;
	}
	/* 必填参数， 文件系统根目录路径 */
	if (f_argv[1] == NULL)
	{
		show_help();
		fprintf(stderr, "Attention: Please use the indispensable argument: -r <root_dir>\n\n");
		return -1;
	}
	
	sprintf(server_settings_dataname, "%s/full_server.db", datapath);

	if (!opendb().ok())
	{
		show_help();
		fprintf(stderr, "Attention: database open failed.\n\n");
		return -1;
	}

	/* 后台运行，fork一个进程，终止父进程 */
	if (daemon)
	{
		pid_t pid = fork();
		if (pid < 0)
			return -1;
		if (pid > 0)
			return 0;
	}

	/* 写进程ID到pidfile */
	FILE *fp_pidfile = fopen(pidfile, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);

	/* 派生工作进程 */
	pid_t worker_process_pid = fork();

	if (worker_process_pid < 0)	//error
	{
		fprintf(stderr, "Error: %s:%d\n", __FILE__, __LINE__);
		return -1;
	}
	/* 父进程 */
	if (worker_process_pid > 0)
	{
		/* 处理kill信号 */
		kill_signal_register(&kill_signal_master);
		/* 如果子进程终止，重新派生子进程 */
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

	/* 处理kill信号 */
	kill_signal_register(&kill_signal_worker);

	/* FUSE线程启动 */
	pthread_t pid;
	pthread_create(&pid, NULL, fuse_init, f_argv);

	/* libevent启动 */
	httpserver_init(listen, port, timeout);

	pthread_join(pid, NULL);

	return 0;
}
