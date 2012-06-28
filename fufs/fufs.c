#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "fufs.h"
#include "op.h"

static int u_fs_getattr(const char *path, struct stat *stbuf)
{

	u_fs_file_directory *attr=malloc(sizeof(u_fs_file_directory));
    memset(stbuf, 0, sizeof(struct stat));
    if(op_open(path,attr)==-1){
    	free(attr);
		return -ENOENT; 
    }

    if (attr->flag==2) {
    	stbuf->st_mode = S_IFDIR | 0666;
    } else if (attr->flag==1) {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_size = attr->fsize;
	}
	
	free(attr);
	return 0;

}

static int u_fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
	u_fs_disk_block *content=malloc(sizeof(u_fs_disk_block));
	u_fs_file_directory *attr=malloc(sizeof(u_fs_file_directory));
	if(op_open(path,attr)==-1){
		goto err;
	}
    long start_blk=attr->nStartBlock;
    if(attr->flag==1)
    	goto err;    	
    if(op_read_blk(start_blk, content))
		goto err;
		
	filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
       	
	u_fs_file_directory *dirent=(u_fs_file_directory*)content->data;
	int position=0;
	char name[MAX_FILENAME + MAX_EXTENSION + 2];		
	while( position<content->size ){
	
		strcpy(name,dirent->fname);
		if(strlen(dirent->fext)!=0){
			strcat(name,".");
			strcat(name,dirent->fext);
		}		
        if (dirent->flag !=0 && filler(buf, name, NULL, 0))
            break;

		dirent++;
		position+=sizeof(u_fs_file_directory);
	}
	
	free(attr);
	free(content);
	return 0;
err:
    free(attr);
    free(content);
    return -ENOENT;
}

static int u_fs_open(const char *path, struct fuse_file_info *fi)
{   
    int res;
	u_fs_file_directory *attr=malloc(sizeof(u_fs_file_directory));
    res = op_open(path,attr);
    if (res == -1)
        return -errno;

    return 0;
   
}

static int u_fs_mkdir(const char *path, mode_t mode)
{
	int res=op_create(path, 2);
	return res;
}

static int u_fs_rmdir(const char *path)
{
	int res=op_rm(path,2);
    return res;
}

static int u_fs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res=op_create(path, 1);
	return res;
}

static int u_fs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    
	u_fs_file_directory *attr=malloc(sizeof(u_fs_file_directory));
    if(op_open(path,attr)==-1) {
    	free(attr);
    	return -ENOENT;
    }
    if(attr->flag==2 ){
    	free(attr);
    	return -EISDIR;
    }
    
    u_fs_disk_block *content;    
    content=malloc(sizeof(u_fs_disk_block));
    
	if(op_read_blk(attr->nStartBlock, content)==-1){
		free(attr);
		free(content);
    	return -1;
	}	
	if( offset < attr->fsize){
		if (offset + size > attr->fsize )
             size = attr->fsize - offset;		
	} else
		size=0;
	int temp=size;
	char *pt=content->data;
	pt+=offset;
	strcpy(buf, pt);
	temp -= content->size;	
	while (temp > 0) {
		if(op_read_blk(content->nNextBlock, content)==-1)
			break;
		strcat(buf,content->data);	
		temp -= content->size;
	}
	free(content);
	free(attr);
    return size;
}

static int u_fs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
	u_fs_file_directory *attr = malloc( sizeof(u_fs_file_directory));
	op_open(path,attr);
    long start_blk=attr->nStartBlock;
	
    if (start_blk == -1){
    	free(attr);
        return -errno;
    }   
    if( offset > attr->fsize){
    	free(attr);
    	return -EFBIG;
    }

	u_fs_disk_block *content = malloc( sizeof(u_fs_disk_block));
	int org_offset=offset;
	int ret=0;
	int total=0;
	long* next_blk=malloc(sizeof(long));
	int num;

	while(1){
		if( op_read_blk(start_blk, content)==-1){
			size=-errno;
			goto exit;
		}
		if(offset<=content->size)
			break;		
		offset-=content->size;
		start_blk=content->nNextBlock;
	}
	
	char* pt = content->data;	
	pt+=offset;		
	ret = (MAX_DATA_IN_BLOCK - offset < size ? MAX_DATA_IN_BLOCK -offset : size); 		 	
	strncpy(pt,buf,ret);
	buf+=ret;
	content->size+=ret;
	total+=ret;
	size-=ret;
	if(size>0){

 		num = op_search_free_blk( size/MAX_DATA_IN_BLOCK+1,next_blk);
		if( num == -1)
			return -errno;
		content->nNextBlock=*next_blk;
		op_write_blk(start_blk,content);
		int i;
		while(1){
			for(i=0; i<num; ++i){
				ret = (MAX_DATA_IN_BLOCK < size ? MAX_DATA_IN_BLOCK : size);
				content->size = ret;
				strncpy(content->data,buf,ret);
				buf+=ret;
				size-=ret;
				total+=ret;
				if(size==0)
					content->nNextBlock=-1;
				else
					content->nNextBlock=*next_blk+1;
				op_write_blk(*next_blk,content);
				*next_blk=*next_blk+1;
			}
			if(size==0)
				break;
			num = op_search_free_blk( size/MAX_DATA_IN_BLOCK+1,next_blk);
			if( num == -1)
				return -errno;																		
		}		
	}
	else if(size==0){

		long next_blk;
		next_blk=content->nNextBlock;
		content->nNextBlock=-1;
		op_write_blk(start_blk,content);

		while(next_blk!=-1){
			op_set_blk(next_blk,0);
			op_read_blk(next_blk,content);
			next_blk=content->nNextBlock;
		}
	}
	size = total;

	attr->fsize = org_offset+size;
	if(op_setattr(path,attr)==-1){
		size=-errno;
		goto exit;
	}
	
exit: 
	free(attr);
	free(content);

	return size;
}

static int u_fs_unlink(const char *path)
{
	int res=op_rm(path,1);
    return res;
}

static int u_fs_truncate(const char *path, off_t size)
{
	return 0;
}

static int u_fs_flush (const char * path, struct fuse_file_info * fi)
{
	return 0;
}

void * u_fs_init (struct fuse_conn_info *conn){
	
	FILE * fp=NULL;

	fp=fopen(DISK, "r+");
	if(fp == NULL) {
		fprintf(stderr,"unsuccessful!\n");
		return 0;
	}
    sb *super_block_record=malloc(sizeof(sb));
    fread(super_block_record, sizeof(sb), 1, fp);
	max_filesystem_in_block = super_block_record->fs_size;
	fclose(fp);
	
	return (long*)max_filesystem_in_block;
}

static struct fuse_operations u_fs_oper = {
    .getattr	= u_fs_getattr,
    .readdir	= u_fs_readdir,
    .mkdir      = u_fs_mkdir,
    .rmdir      = u_fs_rmdir,
    .mknod      = u_fs_mknod,
    .read       = u_fs_read,
    .write      = u_fs_write,
	.unlink     = u_fs_unlink,    
    .open	    = u_fs_open,
    .flush	    = u_fs_flush,
	.truncate	= u_fs_truncate,
	.init       = u_fs_init,
};


int main(int argc, char *argv[])
{

    return fuse_main(argc, argv, &u_fs_oper, NULL);
}
