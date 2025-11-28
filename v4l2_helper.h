#pragma once
#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t

#include <linux/videodev2.h>

enum InitStatus { INIT_SUCCESS = 0, INIT_UNSUPPORTED = 1, INIT_FAILURE = 2 };

struct buffer {
  int *start;
  size_t length;
};

struct device {
  int fd;

  enum v4l2_buf_type buf_type;
  enum v4l2_memory mem_type;

  struct v4l2_format format;
  struct v4l2_requestbuffers reqbuf;

  struct buffer *buffer;
  size_t buffer_count;
};

int init_device(char *dev_node, uint32_t device_cap, struct device *device);
// Open the device and verify it supports the required V4L2 capability.

void req_buf(struct device *dev);
/* Allocate buffers via VIDIOC_REQBUFS. */

void errno_exit(const char *s);
/* Print an error message and abort. */

void init_err(int status);
/* Print error text for init_device() failure. */

void mmap_buf(int count, struct device *dev);

void munmap_buf(struct device *dev);

void start_stream(struct device *dev);

void stop_stream(struct device *dev);
