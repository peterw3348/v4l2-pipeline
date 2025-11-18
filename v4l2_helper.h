#pragma once
#include <stdbool.h>
#include <stdint.h>

#include <linux/videodev2.h>

void errno_exit(const char *s);
int initDevice(char *);
bool checkDeviceCap(int fd, uint32_t device_caps);
