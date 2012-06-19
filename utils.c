/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-06-19 19:16
# Filename: utils.c
# Description: 
============================================================================*/
#include <stdio.h>
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
			if (access(dir, F_OK) != 0)	//test exist
				mkdir(dir, 0777);
			dir[i] = '/';
		}
	}
	if (len > 0 && access(dir, F_OK) != 0)
		mkdir(dir, 0777);
}

void show_help()
{
	fprintf(stdout, "this is show help()\n");
	//XXX
}

