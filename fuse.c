/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-09 13:58
# Filename: fullfs.c
# Description: 
============================================================================*/
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "leveldb/iterator.h"
#include "server.h"
#include "database.h"

static int full_fs_getattr(const char* path, struct stat* stbuf)
{
	int res = 0, value_size;
	char* key = strdup(path);
	leveldb::Status s;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
	} else 
	{
		stbuf->st_mode = S_IFREG | 0444;
		key++;
		s = get(key, NULL, &value_size);//FIXME: check key and path
		if (!s.ok())
			res = -ENOENT;
		else
			stbuf->st_size = value_size;
	}

	free(key);
	return res;
}

static int full_fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
				off_t offset, struct fuse_file_info *fi)
{
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	char* key;

	if (strcmp(path, "/") != 0)
		return -ENOENT;
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		//key = strdup(it->key().data());
		//filler(buf, key, NULL, 0);
		//free(key);
		filler(buf, it->key().data(), NULL, 0);
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
	int res = 0, value_size = 0;
	char value[2] = "\0"; 
	char* key = strdup(path);
	leveldb::Status s;

	key++;
	s = put(key, value, value_size);
	if (!s.ok())
		res = -1;
	else
		res = 0;
	
	free(key);
	return res;
}

static int full_fs_read(const char* path, char* buf, size_t size, off_t offset,
				struct fuse_file_info* fi)
{
	int value_size;
	char* key = strdup(path);
	char* value, *value_ptr;
	leveldb::Status s;

	key++;
	s = get(key, &value, &value_size);
	value_ptr = value;	//to free
	if (!s.ok())
	{
		free(value_ptr);
		return -1;
	}

	value += offset;
	strcpy(buf, value);
	
	free(value_ptr);
	return 0;
}

static int full_fs_write(const char* path, const char* buf, size_t size,
				off_t offset, struct fuse_file_info* fi)
{
}

static int full_fs_unlink(const char* path)
{
}

static int full_fs_open(const char* path, struct fuse_file_info* fi)
{
	int res = 0, value_size;
	char* key = strdup(path);
	leveldb::Status s;

	if (strcmp(path, "/") == 0)
		res = 0;
	else
	{
		key++;
		s = get(key, NULL, &value_size);
		if (!s.ok())
			res = -1;
		else
			res = 0;
	}

	free(key);
	return res;
}

static int full_fs_flush(const char* path, struct fuse_file_info* fi)
{
}

static int full_fs_truncate(const char* path, off_t size)
{
	return 0;
}

void* full_fs_init(struct fuse_conn_info* conn)
{
	//init hash_tree
}
/*
static struct fuse_operations full_fs_oper =
{
	.getattr	= full_fs_getattr,
	.readdir	= full_fs_readdir,
//	.mkdir		= full_fs_mkdir,
//	.rmdir		= full_fs_rmdir,
//	.mknod		= full_fs_mknod,
	.read		= full_fs_read,
//	.write		= full_fs_write,
//	.unlink		= full_fs_unlink,
	.open		= full_fs_open,
//	.flush		= full_fs_flush,
//	.truncate	= full_fs_truncate,
//	.init		= full_fs_init,
};
int main(int argc, char* argv[])
{
	return fuse_main(argc, argv, &full_fs_oper, NULL);
}
*/
static struct full_fs_oper : fuse_operations
{
	full_fs_oper()
	{
		getattr	= full_fs_getattr;
		readdir	= full_fs_readdir;
		read	= full_fs_read;
		open	= full_fs_open;
	}
} full_fs_oper_init;

