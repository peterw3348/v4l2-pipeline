#pragma once
#include <stdbool.h>
#include <stdint.h>

int initDevice(char *);
bool checkDeviceCap(int fd, uint32_t device_caps);
