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

#include "database.h"

static int full_fs_getattr(const char* path, struct stat* stbuf)
{
}

static int full_fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
				off_t offset, struct fuse_file_info *fi)
{
}

static int full_fs_mkdir(const char* path, mode_t mode)
{
}

static int full_fs_rmdir(const char* path)
{
}

static int full_fs_mknod(const char* path, mode_t mode, dev_t rdev)
{
}

static int full_fs_read(const char* path, char* buf, size_t size, off_t offset,
				struct fuse_file_info* fi)
{
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
}

static int full_fs_flush(const char* path, struct fuse_file_info* fi)
{
}

static int full_fs_truncate(const char* path, off_t size)
{
}

void* full_fs_init(struct fuse_conn_info* conn)
{
	//init hash_tree
}

static struct fuse_operations full_fs_oper =
{
	.getattr	= full_fs_getattr,
	.readdir	= full_fs_readdir,
	.mkdir		= full_fs_mkdir,
	.rmdir		= full_fs_rmdir,
	.mknod		= full_fs_mknod,
	.read		= full_fs_read,
	.write		= full_fs_write,
	.unlink		= full_fs_unlink,
	.open		= full_fs_open,
	.flush		= full_fs_flush,
	.truncate	= full_fs_truncate,
	.init		= full_fs_init,
};

int main(int argc, char* argv[])
{
	return fuse_main(argc, argv, &full_fs_oper, NULL);
}
