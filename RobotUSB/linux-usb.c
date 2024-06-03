#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "usb.h"

size_t get_devices(struct SerialById* ids)
{
	unsigned int device_count = 0;
	if(access("/dev/serial/by-id", F_OK) != 0)
	{
		printf("No USB devices detected\n");
		return 0;
	}

	if(chdir("/dev/serial/by-id") != 0)
	{
		printf("Failed to access path\n");
		return 0;
	}

	struct dirent* de;
	DIR* dir = opendir(".");
	if(dir == NULL)
	{
		printf("Failed to open by-id path\n");
		return 0;
	}

	while((de = readdir(dir)) != NULL)
	{
		struct stat istat;
		if(stat(de->d_name, &istat) != 0)
			continue;
		
		if(!S_ISCHR(istat.st_mode))
			continue;

		char* path = realpath(de->d_name, NULL);
		if(path != NULL)
		{
			size_t id_len = strlen(de->d_name);
			const char* id_copy = (const char*)malloc(id_len + 1);
			memset(id_copy, 0, id_len); //null at end
			memcpy(id_copy, de->d_name, id_len);
			ids[device_count].id = id_copy;
			ids[device_count].path = path;
			++device_count;
		}	
	}
	closedir(dir);
	return device_count;
}