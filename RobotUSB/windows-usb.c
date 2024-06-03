#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <setupapi.h>
#include <devguid.h>
#include <conio.h>
#include <Windows.h>
#pragma comment (lib, "SetupAPI")
#include "usb.h"

size_t get_devices(struct SerialById* ids)
{
}