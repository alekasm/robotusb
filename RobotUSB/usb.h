#pragma once
#define MAX_SERIAL_DEVICES 32
struct SerialById
{
	const char* id;
	const char* path;
};

size_t get_devices(struct SerialById*);