#pragma once
#include "buffer.h"
#include "conversion.h"
#include <errno.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/**
 * @file v4l2_helper.h
 * @brief Helper utilities for V4L2 device initialization and buffer handling.
 *
 * This module abstracts:
 *   - Opening and configuring V4L2 devices
 *   - Setting formats and frame rate
 *   - Requesting / mapping buffers (REQBUFS, QUERYBUF)
 *   - Starting/stopping streaming
 *   - Safe cleanup of memory-mapped buffers
 */

struct device {
  char *name; ///< Device path, e.g. "/dev/video0"
  int fd;     ///< File descriptor returned by open()

  enum v4l2_buf_type buf_type; ///< Capture or Output buffer type
  enum v4l2_memory mem_type;   ///< Usually V4L2_MEMORY_MMAP

  struct v4l2_format format;
  struct v4l2_requestbuffers reqbuf;

  struct buffer *buffer; ///< Array of mapped buffers
  size_t buffer_count;   ///< Number of buffers
};

/**
 * @brief Open the device, verify its capability, set formats, allocate buffers.
 */
void init_device(char *dev_node, uint32_t device_cap, int width, int height,
                 struct device *dev);

/**
 * @brief Stop streaming, unmap buffers, and close the device.
 */
void deinit_device(struct device *device);

/**
 * @brief Perform VIDIOC_REQBUFS.
 */
void req_buf(struct device *dev);

/**
 * @brief Request, query, and mmap() N buffers.
 */
void mmap_buf(int count, struct device *dev);

/**
 * @brief Unmap previously mapped buffers and free user-space tracking
 * structures.
 */
void munmap_buf(struct device *dev);

/**
 * @brief Request, query, and mmap() N buffers.
 */
void dmabuf(int count, struct device *dev);

/**
 * @brief Unmap previously mapped buffers and free user-space tracking
 * structures.
 */
void undmabuf(struct device *dev);

/**
 * @brief Queue every buffer then start streaming with VIDIOC_STREAMON.
 */
void start_stream(struct device *dev);

/**
 * @brief Stop streaming via VIDIOC_STREAMOFF.
 */
void stop_stream(struct device *dev);

/**
 * @brief Enumerate supported pixel formats, frame sizes, and intervals.
 */
void enum_caps(struct device *dev);

/**
 * @brief Print error & abort — used for fatal V4L2 operations.
 */
static inline void errno_exit(const char *s) {
  fprintf(stderr, "%s error %d %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

/**
 * @brief ioctl() wrapper that retries on EINTR.
 */
static inline int xioctl(int fh, int request, void *arg) {
  int r;
  do {
    r = ioctl(fh, request, arg);
  } while (r == -1 && errno == EINTR);
  return r;
}
