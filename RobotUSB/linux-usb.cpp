#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include "usb.h"

size_t get_devices(struct SerialById *ids)
{
    unsigned int device_count = 0;
    if (access("/dev/serial/by-id", F_OK) != 0)
    {
        printf("No USB devices detected\n");
        return 0;
    }

    if (chdir("/dev/serial/by-id") != 0)
    {
        printf("Failed to access path\n");
        return 0;
    }

    struct dirent *de;
    DIR *dir = opendir(".");
    if (dir == NULL)
    {
        printf("Failed to open by-id path\n");
        return 0;
    }

    while ((de = readdir(dir)) != NULL)
    {
        struct stat istat;
        if (stat(de->d_name, &istat) != 0)
            continue;

        if (!S_ISCHR(istat.st_mode))
            continue;

        char *path = realpath(de->d_name, NULL);
        if (path != NULL)
        {
            //printf("Name: %s, Path: %s\n", de->d_name, path);
            size_t id_len = strlen(de->d_name);
            char* id_copy = (char*)malloc(id_len + 1);
            memset(id_copy, 0, id_len); // null at end
            memcpy(id_copy, de->d_name, id_len);
            ids[device_count].id = id_copy;
            ids[device_count].path = path;
            ++device_count;
            //free(path);
            path = NULL;
        }
    }
    closedir(dir);
    return device_count;
}

int sd_open(const char *device)
{
    return open(device, O_RDWR | O_NOCTTY);
}

int sd_config(int device, struct SerialSettings settings)
{
    // 115200
    struct termios newtermios;
    newtermios.c_cflag = CS8 | CLOCAL | CREAD;
    newtermios.c_iflag = IGNPAR;
    newtermios.c_oflag = 0;
    newtermios.c_lflag = 0;
    newtermios.c_cc[VMIN] = 1;
    newtermios.c_cc[VTIME] = 0;
    cfsetospeed(&newtermios, settings.baud);
    cfsetispeed(&newtermios, settings.baud);
    if (tcflush(device, TCIFLUSH) == -1)
        return -1;
    if (tcflush(device, TCOFLUSH) == -1)
        return -1;
    if (tcsetattr(device, TCSANOW, &newtermios) == -1)
        return -1;
    return 0;
}

int sd_write(int device, const char *buf, size_t size)
{
    return write(device, buf, size);
}

void sd_close(int device)
{
    if (device > -1)
    {
        close(device);
    }
}