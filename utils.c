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
			if (access(dir, F_OK) != 0)
				mkdir(dir, 0777);
			dir[i] = '/';
		}
	}
	if (len > 0 && access(dir, F_OK) != 0)
		mkdir(dir, 0777);
}
