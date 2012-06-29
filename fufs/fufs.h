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
/*超级块*/
typedef struct {
     long fs_size;/*文件系统大小*/                     
     long first_blk;/*首块序号*/                  
     long bitmap;/*位图块大小*/                    
}sb;
/*日志*/
typedef struct {
     char fname[MAX_FILENAME + 1];/*文件名称*/      
     char fext[MAX_EXTENSION + 1];/*扩展*/      
     size_t fsize;/*文件大小*/                     
     long nStartBlock;/*首块位置*/                
     int flag;/*类型，代表文件或目录*/                      
}u_fs_file_directory;
/*数据块*/
typedef struct {
     size_t size;/*块内已用大小*/                     
     long nNextBlock;/*下一个块序号*/                 
     char data[MAX_DATA_IN_BLOCK];/*存储数据空间*/    
}u_fs_disk_block ;

typedef long int ufs_DIR;             
							  
#endif
