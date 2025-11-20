#pragma once
#include <stdbool.h>    // bool
#include <stdint.h>     // uint32_t
#include <stddef.h>     // size_t

#include <linux/videodev2.h>

enum InitStatus
{
	INIT_SUCCESS        = 0,
    INIT_UNSUPPORTED    = 1,
	INIT_FAILURE        = 2
};

typedef struct
{
    int *   start;
    size_t  length;
} Buffer;

typedef struct
{
    int                 fd;

    enum v4l2_buf_type  buftype;
    enum v4l2_memory    memtype;

    struct v4l2_format  format;
    struct v4l2_requestbuffers reqbuf;
    
    Buffer *            buffer;
    size_t              buffer_count;
} Device;

void errno_exit(const char *s);
int initDevice(char* dev_node, uint32_t device_cap, Device * device);
void reqBuf(Device * dev, int count);
void init_err(int status);
