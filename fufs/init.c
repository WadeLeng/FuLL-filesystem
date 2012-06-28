#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include "fufs.h"

int main(void){
	
	FILE * fp=NULL;

	fp=fopen(DISK, "r+");
	if(fp == NULL) {
		fprintf(stderr,"open diskimg unsuccessful!\n");
		return 0;
	}
	
	sb *super_block_record=malloc(sizeof(sb));

	fseek(fp, 0, SEEK_END);	
	super_block_record->fs_size = ftell(fp)/BLOCK_BYTES;	
	super_block_record->first_blk = 1 + MAX_BITMAP_IN_BLOCK;
	super_block_record->bitmap = MAX_BITMAP_IN_BLOCK;
		
	if(fseek(fp, 0, SEEK_SET )!=0)
		fprintf(stderr,"unsuccessful!\n");	
	fwrite(super_block_record, sizeof(sb), 1, fp);
	if(fseek(fp, 512, SEEK_SET )!=0)
		fprintf(stderr,"unsuccessful!\n");	

	char a[180];
	memset(a,-1,180);	
	fwrite(a, 180, 1, fp);
	int temp=0x80000000;
	int* pt=&temp;
	fwrite(pt, sizeof(int), 1, fp);
	char b[328];
	memset(b,0,328);
	fwrite(b,328,1,fp);
	int total = (MAX_BITMAP_IN_BLOCK-1)*BLOCK_BYTES;
	char rest[total];
	memset(rest, 0, total);
	fwrite(rest, total, 1, fp);

	fseek(fp, BLOCK_BYTES * (MAX_BITMAP_IN_BLOCK+1), SEEK_SET);
    u_fs_disk_block *root=malloc(sizeof(u_fs_disk_block));
    root->size= 0;
    root->nNextBlock=-1;
    root->data[0]='\0';
    fwrite(root, sizeof(u_fs_disk_block), 1, fp);
    			
	fclose(fp);
	printf("initialize successful!\n");
	return 0;
}

