#include <errno.h>    // for errno
#include <fcntl.h>    // for open()
#include <inttypes.h> // for uint32_t
#include <stdbool.h>  // for bool
#include <stdio.h>
#include <stdlib.h>    // for EXIT_FAILURE
#include <string.h>    // for strerror(), memset()
#include <sys/ioctl.h> // for ioctl()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

struct bit_to_cap_name {
  uint32_t bit;
  const char *name;
};

static const struct bit_to_cap_name cap_table[] = {
    {V4L2_CAP_VIDEO_CAPTURE, "VIDEO CAPTURE"},
    {V4L2_CAP_VIDEO_OUTPUT, "VIDEO OUTPUT"},
    {V4L2_CAP_VIDEO_OVERLAY, "VIDEO OVERLAY"},
    {V4L2_CAP_VBI_CAPTURE, "VBI CAPTURE"},
    {V4L2_CAP_VBI_OUTPUT, "VBI OUTPUT"},
    {V4L2_CAP_SLICED_VBI_CAPTURE, "SLICED VBI CAPTURE"},
    {V4L2_CAP_SLICED_VBI_OUTPUT, "SLICED VBI OUTPUT"},
    {V4L2_CAP_RDS_CAPTURE, "RDS CAPTURE"},
    {V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "VIDEO OUTPUT OVERLAY"},
    {V4L2_CAP_HW_FREQ_SEEK, "HW FREQ SEEK"},
    {V4L2_CAP_RDS_OUTPUT, "RDS OUTPUT"},
    {V4L2_CAP_VIDEO_CAPTURE_MPLANE, "VIDEO_CAPTURE_MPLANE"},
    {V4L2_CAP_VIDEO_OUTPUT_MPLANE, "VIDEO_OUTPUT_MPLANE"},
    {V4L2_CAP_VIDEO_M2M_MPLANE, "VIDEO_M2M_MPLANE"},
    {V4L2_CAP_VIDEO_M2M, "VIDEO_M2M"}};

/**
 * init_device() - Open a V4L2 device and verify the required capability.
 *
 * @dev_node:   Path to the device node (e.g. "/dev/video0").
 * @device_cap: Required V4L2_CAP_* bit (e.g. V4L2_CAP_VIDEO_CAPTURE).
 * @device:     Output Device structure to initialize.
 *
 * Returns: INIT_SUCCESS on success, or an INIT_* error code indicating
 *          why initialization failed.
 *
 * This function:
 *   1. Opens the provided device node with O_RDWR | O_NONBLOCK.
 *   2. Queries device capabilities via VIDIOC_QUERYCAP.
 *   3. Checks whether the device advertises the required @device_cap.
 *   4. If supported, assigns the corresponding V4L2 buffer type
 *      (V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_BUF_TYPE_VIDEO_OUTPUT, etc.).
 *   5. Initializes device->format.type, which is required before calling
 *      VIDIOC_G_FMT or VIDIOC_S_FMT.
 *
 * The Device struct is zero-initialized on entry. On failure, this function
 * returns an INIT_* code instead of exiting the program.
 */
int init_device(char *dev_node, uint32_t device_cap, Device *device) {
  // fd, buf_type
  printf("init %s\n", dev_node);
  int initStatus = INIT_SUCCESS;
  *device = (typeof(*device)){0};

  device->fd = open(dev_node, O_RDWR /* required */ | O_NONBLOCK, 0);
  if (device->fd == -1) {
    errno_exit("OPEN");
  }
  printf("%s opened as %d\n", dev_node, device->fd);

  struct v4l2_capability caps;
  ioctl(device->fd, VIDIOC_QUERYCAP, &caps);

  printf("Device Caps:\n");
  initStatus = INIT_UNSUPPORTED;
  for (int i = 0; i < sizeof(cap_table) / sizeof(struct bit_to_cap_name); ++i) {
    if (cap_table[i].bit & caps.device_caps) // check supported device_caps
    {
      // dev_cap matches
      printf("%s\n", cap_table[i].name);
      if (cap_table[i].bit == device_cap) // check if we can configure device
      {
        switch (device_cap) {
        case V4L2_CAP_VIDEO_CAPTURE:
          device->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          initStatus = INIT_SUCCESS;
          break;
        case V4L2_CAP_VIDEO_OUTPUT:
          device->buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
          initStatus = INIT_SUCCESS;
          break;
        default: // unimplemented
          initStatus = INIT_FAILURE;
          break;
        }
      }
    }
  }
  if (device->buf_type) {
    // Kernel rejects format with empty type. How do we know type?
    // If we know the device caps, we will also know supported
    // buf types
    device->format.type = device->buf_type;
  }
  return initStatus;
}

void req_buf(Device *dev, int count) {
  struct v4l2_requestbuffers buf = {0};
  buf.count = count;
  buf.type = dev->buf_type;
  buf.memory = dev->mem_type;
  if (-1 == ioctl(dev->fd, VIDIOC_REQBUFS, &buf)) {
    errno_exit("VIDIOC_REQBUFS");
  }
}

/**
 * errno_exit() - Print an error message and abort.
 *
 * @s: Short tag string describing the failed operation (e.g. "OPEN",
 *     "VIDIOC_QUERYCAP", etc.).
 *
 * This helper prints the provided tag, the current errno value, and the
 * corresponding strerror() message to stderr. The process then terminates
 * with EXIT_FAILURE. This is intended for fatal V4L2 or system call
 * failures where recovery is not attempted.
 */
void errno_exit(const char *s) {
  fprintf(stderr, "%s error %d %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

/**
 * init_err() - Print a human-readable error message for init_device().
 *
 * @status: Initialization return code (INIT_SUCCESS, INIT_UNSUPPORTED,
 *          INIT_FAILURE).
 *
 * This function prints a short diagnostic error message describing why
 * init_device() failed. It does not exit the program. The caller is responsible
 * for handling the failure case.
 */
void init_err(int status) {
  switch (status) {
  case INIT_UNSUPPORTED:
    fprintf(stderr, "ERROR: Device does not have provided capability");
  case INIT_FAILURE:
    fprintf(stderr, "ERROR: Init");
  }
}
