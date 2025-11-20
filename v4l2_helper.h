#pragma once
#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t

#include <linux/videodev2.h>

enum InitStatus { INIT_SUCCESS = 0, INIT_UNSUPPORTED = 1, INIT_FAILURE = 2 };

typedef struct {
  int *start;
  size_t length;
} Buffer;

typedef struct {
  int fd;

  enum v4l2_buf_type buf_type;
  enum v4l2_memory mem_type;

  struct v4l2_format format;
  struct v4l2_requestbuffers reqbuf;

  Buffer *buffer;
  size_t buffer_count;
} Device;

int init_device(char *dev_node, uint32_t device_cap, Device *device);
// Open the device and verify it supports the required V4L2 capability.

void req_buf(int count, Device *dev);
/* Allocate buffers via VIDIOC_REQBUFS. */

void errno_exit(const char *s);
/* Print an error message and abort. */

void init_err(int status);
/* Print error text for init_device() failure. */

void mmap_buf(int count, Device *dev);

void munmap_buf(Device *dev);
