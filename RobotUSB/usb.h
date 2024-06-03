#pragma once
#define MAX_SERIAL_DEVICES 32
#include <stdint.h>

struct SerialById
{
	const char* id;
	const char* path;
};

enum SerialParity { SP_NOPARITY };
enum SerialStopBits { SP_ONESTOPBIT };
struct SerialSettings
{
	size_t baud;
	enum SerialParity parity;
	enum SerialStopBits stopbits;
};

size_t get_devices(struct SerialById*);
int sd_open(const char* device);
int sd_config(int device, struct SerialSettings);
int sd_write(int device, const char* buf, size_t size);
void sd_close(int device);
