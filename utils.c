/*============================================================================
# Author: Wade Leng
# E-mail: wade.hit@gmail.com
# Last modified: 2012-04-07 16:53
# Filename: utils.c
# Description: 
============================================================================*/
#include <unistd.h>

void create_multilayer_dir(char *dir)
{
	int i, len;

	len = strlen(dir);
	for (i = 0; i < len; i++)
	{
		if (dir[i] == '/')
		{
			dir[i] = '\0';
			if (access(str, F_OK) != 0)
				mkdir(dir, 0777);
			dir[i] = '/';
		}
	}
	if (len > 0 && access(dir, F_OK) != 0)
		mkdir(dir, 0777);
}
