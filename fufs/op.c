#include "fufs.h"
#include "op.h" 

#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>    
#include <errno.h>
/*对日志的操作，删除文件时设标志位flag为0，写文件时更改文件大小*/
int op_setattr(const char* org_path, u_fs_file_directory* attr)
{
	int res;
	char* path;
	char *p,*q;
	u_fs_disk_block* content;
	u_fs_file_directory *a=malloc(sizeof(u_fs_file_directory));
	content = malloc(sizeof(u_fs_disk_block)); 	
	path = strdup(org_path);	
	p=path;
	if (!p){/*无足够内存*/
		res = -1;
		goto exit;
	}
	p++;
	q = strchr(p, '/');		
	if( q != NULL ){
		*q = '\0';/*指向目录名*/ 
		q++;
		p=q;/*指向文件名*/      
		if(op_open(path,a)==-1){
			res = -ENOENT;
			goto exit;
		}
	}
	else {
		if(op_open("/",a)==-1){
			res = -ENOENT;
			goto exit;
		}
    }
		
	q = strchr(p, '.');
	if( q!=NULL ){ 
		*q = '\0'; 
		q++;    
	}			


    long start_blk=a->nStartBlock;

	if(start_blk==-1){
		res = -1;
		goto exit;
	}	
	if(op_read_blk(start_blk, content)==-1){
		res = -1;
		goto exit;
	}
	u_fs_file_directory *dirent=(u_fs_file_directory*)content->data;
	int position=0;	
	while( position<content->size ){
		
		if( dirent->flag !=0 && strcmp(p,dirent->fname)==0 && ( q==NULL || strcmp(q,dirent->fext)==0 ) ){
			strcpy(dirent->fname, attr->fname);
			strcpy(dirent->fext, attr->fext); 
			dirent->fsize = attr->fsize;
			dirent->nStartBlock = attr->nStartBlock;
			dirent->flag = attr->flag;
			res=0;
			break;
		}

		dirent++;
		position+=sizeof(u_fs_file_directory);
	}
	if( op_write_blk(start_blk, content)==-1){
		res = -1;
		goto exit;
	}
exit:
	free(a);
	free(path);
	free(content);
	return res;			
}
/*返回所打开文件或目录的日志*/
int op_open(const char * org_path, u_fs_file_directory *attr){
		
	char *p,*q,*path;
	long start_blk;
	sb* sb_record;
	u_fs_disk_block *content;    
    content=malloc(sizeof(u_fs_disk_block));
    
	path = strdup(org_path);
	p=path;		

	if(op_read_blk(0,content)==-1)/*读超级块*/
		goto err; 	      	
    sb_record=(sb*)content;    
    start_blk=sb_record->first_blk;

    if(strcmp(org_path,"/")==0){ /*打开根目录*/
		attr->flag=2;
		attr->nStartBlock=start_blk;
		goto ok;
	}

	if (!p)
		goto err;		
	p++;
	
	q = strchr(p, '/');		
	if( q!=NULL ){
		path++; 
		*q = '\0'; 
		q++;
		p=q;     
	}
	
	q = strchr(p, '.');
	if( q!=NULL ){ 
		*q = '\0'; 
		q++;     
	}
	 
    if(op_read_blk(start_blk,content)==-1){
		goto err;
	}
	u_fs_file_directory *dirent=(u_fs_file_directory*)content->data;
		
	if(*path=='/')
		goto file;
	int offset=0;	
	while( offset<content->size ){
		
		if(strcmp(dirent->fname,path)==0 && dirent->flag==2 ){
			start_blk=dirent->nStartBlock;
			break;
		}

		dirent++;
		offset+=sizeof(u_fs_file_directory);
	}
	if(offset==content->size)
		goto err;
	
	if(op_read_blk(start_blk, content)==-1)
		goto err;	
	
	dirent=(u_fs_file_directory*)content->data;
	
file:
	offset=0;		
	while( offset<content->size ){
		
		if( dirent->flag !=0 && strcmp(dirent->fname,p)==0 && ( q==NULL || strcmp(dirent->fext,q)==0 ) ){
			 
			start_blk=dirent->nStartBlock;
			strcpy(attr->fname, dirent->fname);
			strcpy(attr->fext, dirent->fext);
			attr->fsize=dirent->fsize;
			attr->nStartBlock=dirent->nStartBlock;
			attr->flag=dirent->flag;
			
			goto ok;
		}

		dirent++;
		offset+=sizeof(u_fs_file_directory);
	}	
	goto err;
ok:
		free(content);

		return 0;
err:
    	free(content);

    	return -1;

}
/*统一创建目录和文件*/
int op_create(const char* org_path, int flag){
	long blk;
	char *p,*q,*path;
	
	path = strdup(org_path);
	p=path;

	if (!p)
		return -errno;
	
	u_fs_file_directory* attr=malloc(sizeof(u_fs_file_directory));
		
	p++;
	
	q = strchr(p, '/');
	
	if(flag==2 && q!=NULL) 
		return -EPERM;	
	else if( q!=NULL ){
		*q = '\0';	
		q++;
		p=q;   
		if(op_open(path,attr)==-1){
			free(attr);
			return -ENOENT;
		}
	}

	if(q==NULL){ 
		if(op_open("/",attr)==-1){
			free(attr);
			return -ENOENT;
		}
    }
	
	if(flag==1){
		q = strchr(p, '.');
		if( q!=NULL ){ 
			*q = '\0'; 
			q++;     
		}	
	}

	if(flag==1){		
		if( strlen(p) > MAX_FILENAME 
		||  ( q != NULL && strlen(q) > MAX_EXTENSION) ){
			free(attr);	
			return -ENAMETOOLONG; 
		}
	}
	else if( flag==2 ){
		if(strlen(p)>MAX_FILENAME ){
			free(attr);
			return -ENAMETOOLONG;
		} 
	}
	
	long p_dir;

	p_dir=attr->nStartBlock;
	free(attr);	
	
	if(p_dir == -1)
		return -ENOENT;
	
	u_fs_disk_block *content=malloc(sizeof(u_fs_disk_block));
	if(op_read_blk(p_dir, content)==-1)
		return -ENOENT;	
	u_fs_file_directory *dirent=(u_fs_file_directory*)content->data;
	
	int offset=0;
	int position=content->size;

	while( offset<content->size ){
		if(flag==0)
			position=offset;		
		else if( flag==1 &&  dirent->flag==1 && strcmp(p,dirent->fname)==0 
		         && ( (q ==NULL && strlen(dirent->fext)==0 ) || (q!=NULL && strcmp(q,dirent->fext)==0) ) )
				return -EEXIST;
		else if ( flag==2 && dirent->flag==2 && strcmp(p,dirent->fname)==0 ) 
			   	return -EEXIST;
		
		dirent++;
		offset+=sizeof(u_fs_file_directory);
	}
		      
	if(position==content->size){
		if(content->size > MAX_DATA_IN_BLOCK){
			goto new_blk;			
		}
		else{
			content->size+=sizeof(u_fs_file_directory); 
		}	
	}
	else{
		offset=0;
		dirent=(u_fs_file_directory*)content->data;
		while(offset<position)
			dirent++;
	}
	strcpy(dirent->fname,p);
	if(flag ==1 && q!=NULL)
		strcpy(dirent->fext,q);
	dirent->fsize=0;
	dirent->flag=flag;
	long *temp=malloc(sizeof(long));	
	if(op_search_free_blk(1,temp)==1)
		dirent->nStartBlock=*temp;
	else
		return -errno;
	free(temp);	

	op_write_blk(p_dir,content); 

exit:	
	content->size=0;
	content->nNextBlock=-1;
	strcpy(content->data,"\0");	
	op_write_blk(dirent->nStartBlock,content);
    
    free(content);
    free(path);    
	return 0;

new_blk:
	
	if(op_search_free_blk(1,temp)==1)
		blk=*temp;
	else
		return -errno;
	free(temp);
	content->nNextBlock=blk;

	op_write_blk(p_dir,content);

	content->size=sizeof(u_fs_file_directory);
	content->nNextBlock=-1;
	dirent=(u_fs_file_directory*)content->data;
		strcpy(dirent->fname,p);
	if(flag ==1)
		strcpy(dirent->fext,q);
	dirent->fsize=0;
	dirent->flag=flag;
	long *t=malloc(sizeof(long));	
	if(op_search_free_blk(1,t)==1)
		dirent->nStartBlock=*t;
	else
		return -errno;
	free(t);	
	op_write_blk(blk,content);
	goto exit;
}
/*统一删除文件和目录，自动释放相应数据块，设日志flag为0*/
int op_rm(const char *org_path,int flag)
{
	u_fs_file_directory *attr=malloc(sizeof(u_fs_file_directory));
    if(op_open(org_path,attr)==-1){
    	free(attr);
		return -ENOENT; 
    }
    if(flag==1 && attr->flag==2){
    	free(attr);
		return -EISDIR; 
    }
    else if (flag==2 && attr->flag ==1){
    	free(attr);
		return -ENOTDIR; 
    }
	u_fs_disk_block* content=malloc(sizeof(u_fs_disk_block));    

    if(flag==1){
    	long next_blk=attr->nStartBlock;
		
		while(next_blk!=-1){
			op_set_blk(next_blk,0);
			op_read_blk(next_blk,content);
			next_blk=content->nNextBlock;
		}
    }
    else if( !is_empty_dir(org_path)){ 
			free(attr);
			free(content);
			return -ENOTEMPTY;
	}
		
    attr->flag=0;
    if(op_setattr(org_path,attr)==-1){
		free(attr);
		free(content);
    return -1;
	}
	
	free(attr);
	free(content);
	return 0;
}

int is_empty_dir(const char* path){
	
	u_fs_disk_block *content=malloc(sizeof(u_fs_disk_block));
	u_fs_file_directory *attr=malloc(sizeof(u_fs_file_directory));
	if(op_open(path,attr)==-1){
		goto noEmpty;
	}
    long start_blk;
    start_blk=attr->nStartBlock;
    if(attr->flag==1)
    	goto noEmpty;    	
    if(op_read_blk(start_blk, content))
		goto noEmpty;
		
       	
	u_fs_file_directory *dirent=(u_fs_file_directory*)content->data;
	int position=0;
		
	while( position<content->size ){
		
        if (dirent->flag !=0 ) 
            goto noEmpty;
		dirent++;
		position+=sizeof(u_fs_file_directory);
	}
	
	free(attr);
	free(content);
	return 1;
noEmpty:
    free(attr);
    free(content);
    return 0;

}
/*遍历块，返回块的状态*/
int op_read_blk(long blk,u_fs_disk_block * content){
	
	FILE* fp;
	fp=fopen(DISK, "r+");
	if(fp==NULL)
		return -1;

	if(fseek(fp, blk*BLOCK_BYTES, SEEK_SET)!=0)
		goto err;
	fread(content, sizeof(u_fs_disk_block), 1, fp);
	if(ferror (fp) || feof(fp))
		goto err;

	fclose(fp);
	return 0;
	
err:
	fclose(fp);
	return -1;
}

int op_write_blk(long blk,u_fs_disk_block * content);
int op_write_blk(long blk,u_fs_disk_block * content){
	FILE* fp;
	fp=fopen(DISK, "r+");
	if(fp==NULL)
		return -1;
	if(fseek(fp, blk*BLOCK_BYTES, SEEK_SET)!=0)
		goto err;
	fwrite(content, sizeof(u_fs_disk_block), 1, fp);
	if(ferror (fp) || feof(fp))
		goto err;

	fclose(fp);
	return 0;
	
err:
	fclose(fp);
	return -1;
} 
 
int op_search_free_blk(int num,long* start_blk){	

    *start_blk=1+MAX_BITMAP_IN_BLOCK+1;
    int temp=0;
	FILE* fp=NULL;
	fp=fopen(DISK, "r+");
	if(fp==NULL)
		return 0;
	int start,left;
	unsigned int mask,f; 
    int *flag;
	
	int max=0;
	long max_start=-1;
	  
	while(*start_blk< max_filesystem_in_block-1){
			
		start = *start_blk/8;
		left = *start_blk%8;
		mask=1;
		mask<<=left;	
		fseek(fp,BLOCK_BYTES+start,SEEK_SET);
		flag=malloc(sizeof(int));
		fread(flag,sizeof(int),1,fp);
		f=*flag;
		for(temp=0; temp<num; temp++){
			if( (f&mask) == mask)
				break;
			if( (mask &0x80000000 )== 0x80000000){
				fread(flag,sizeof(int),1,fp);
				f=*flag;
				mask=1;
			}
			else
				mask<<=1;
		}
		if(temp>max){
			max=temp;
			max_start=*start_blk;
		}
		if(temp==num)
			break;
		
		*start_blk=(temp+1)+*start_blk;
		temp=0;

	}
	*start_blk=max_start;
	fclose(fp);	
	int j=max_start;
	int i;
	for(i=0;i<max;++i){
		if(op_set_blk(j++,1)==-1)
			return -1;
	}
	
	return max;
	   
}

int op_set_blk(long blk,int flag){

	if(blk==-1)
		return -1;
	FILE* fp=NULL;
	fp=fopen(DISK, "r+");
	if(fp==NULL)
		return -1;
	int start=blk/8;
	int left=blk%8;
	int f;
	int mask=1;
	mask<<= left;	
	fseek(fp,BLOCK_BYTES+start,SEEK_SET);
	int *temp=malloc(sizeof(int));
	fread(temp,sizeof(int),1,fp);
	f=*temp;
	if(flag)
		f|= mask;
	else   
		f&= ~mask;
	
	*temp=f;	
	fseek(fp,BLOCK_BYTES+start,SEEK_SET);
	fwrite(temp,sizeof(int),1,fp);	
	fclose(fp);
	
	return 0;
}
