
#ifndef OP_H
#define OP_H

#include "fufs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int op_create(const char *, int flag);
int op_open(const char * org_path, u_fs_file_directory *attr);
int op_setattr(const char* org_path, u_fs_file_directory * attr);
int op_rm(const char *path,int flag);
int is_empty_dir(const char* path);
int op_read_blk(long blk,u_fs_disk_block * content);
int op_write_blk(long blk,u_fs_disk_block * content);
int op_search_free_blk(int num,long* start_blk);
int op_set_blk(long blk,int flag);

#endif
