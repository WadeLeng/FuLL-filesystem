#ifndef FUFS_H
#define FUFS_H

#include <stddef.h>

#define DISK "/home/sony/fufs/diskimg" 

#define MAX_BITMAP_IN_BLOCK 1280 
#define MAX_FILENAME 8          
#define MAX_EXTENSION 3        
#define MAX_DATA_IN_BLOCK 504		
#define BLOCK_BYTES 512       

long max_filesystem_in_block; 

typedef struct {
     long fs_size;                     
     long first_blk;                  
     long bitmap;                    
}sb;

typedef struct {
     char fname[MAX_FILENAME + 1];      
     char fext[MAX_EXTENSION + 1];      
     size_t fsize;                     
     long nStartBlock;                
     int flag;                      
}u_fs_file_directory;

typedef struct {
     size_t size;                     
     long nNextBlock;                 
     char data[MAX_DATA_IN_BLOCK];    
}u_fs_disk_block ;

typedef long int ufs_DIR;             
							  
#endif
