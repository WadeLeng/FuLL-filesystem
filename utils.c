/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:16
# Filename: utils.c
# Description: 
============================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

void create_multilayer_dir(char *dir)
{
	int i, len;

	len = strlen(dir);
	for (i = 0; i < len; i++)
	{
		if (dir[i] == '/')
		{
			dir[i] = '\0';
			/* 检测dir是否存在 */
			if (access(dir, F_OK) != 0)
				mkdir(dir, 0777);
			dir[i] = '/';
		}
	}
	if (len > 0 && access(dir, F_OK) != 0)
		mkdir(dir, 0777);
}

void show_help()
{
	char *help_msg = (char*) "--------------------------------------------------------------------------------------------------\n"
		"FUSE + libevent + leveldb filesystem-----FuLL filesystem v1.0\n\n"
		"This is free software, and you are welcome to modify and redistribute it under the New BSD License\n"
		"\n"
		"-l <ip_addr>  interface to listen on, default is 0.0.0.0\n"
		"-p <num>      TCP port number to listen on (default: 12345)\n"
		"-x <path>     database directory \n"
		"-t <second>   keep-alive timeout for an http request (default: 60)\n"
		"-c <num>      the maximum cached data nodes to be cached (default: 100MB)\n"
		"-i <file>     save PID in <file> (default: /tmp/full_fs.pid)\n"
		"-r <root_dir> the filesystem root dir\n"
		"-d            run as a daemon\n"
		"-h            print this help and exit\n\n"
		"Use command \"killall -9 full_server\", \"pkill -9 full_server\" to stop FuLL filesystem.\n"
		"Use command \"fusermount -u <root_dir>\" to unmount filesystem.\n"
		"--------------------------------------------------------------------------------------------------\n"
		"\n";
	fprintf(stderr, help_msg, strlen(help_msg));
}

char *urldecode(char *input_str) 
{
	int len = strlen(input_str);
	char *str = strdup(input_str);
	
    char *dest = str; 
    char *data = str; 

    int value; 
    int c; 

    while (len--) { 
        if (*data == '+') 
            *dest = ' '; 
 
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1)) && isxdigit((int) *(data + 2))) 
        { 
            c = ((unsigned char *)(data+1))[0]; 
            if (isupper(c)) 
                c = tolower(c); 
            value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16; 
            c = ((unsigned char *)(data+1))[1]; 
            if (isupper(c)) 
               c = tolower(c); 
            value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10; 

            *dest = (char)value ; 
            data += 2; 
            len -= 2; 
		} else {
            *dest = *data; 
	    }
        data++; 
        dest++; 
    } 
    *dest = '\0'; 
    return str; 
}
