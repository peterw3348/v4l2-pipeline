#include <fcntl.h>    // for open()
#include <inttypes.h> // for uint32_t
#include <stdbool.h>  // for bool
#include <stdio.h>
#include <sys/ioctl.h> // for ioctl()
#include <sys/mman.h>  // for mmap()
#include <unistd.h>    // for close()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

#define YUYV_BYTES_PER_PIXEL 2

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
 *   5. Initializes dev->format.type, which is required before calling
 *      VIDIOC_G_FMT or VIDIOC_S_FMT.
 *
 * The Device struct is zero-initialized on entry. On failure, this function
 * returns an INIT_* code instead of exiting the program.
 */
void init_device(char *dev_node, uint32_t device_cap, int width, int height,
                 struct device *dev) {
  *dev = (typeof(*dev)){0};
  dev->name = dev_node;
  printf("%s: init\n", dev->name);
  dev->fd = open(dev_node, O_RDWR /* required */ | O_NONBLOCK, 0);
  if (dev->fd == -1) {
    errno_exit("open");
  }
  printf("%s: opened as %d\n", dev_node, dev->fd);

  struct v4l2_capability caps;
  xioctl(dev->fd, VIDIOC_QUERYCAP, &caps);

  printf("Device Caps:\n");
  for (int i = 0; i < sizeof(cap_table) / sizeof(struct bit_to_cap_name); ++i) {
    if (cap_table[i].bit & caps.device_caps) // check supported device_caps
    {
      // dev_cap matches
      printf("%s\n", cap_table[i].name);
      if (cap_table[i].bit == device_cap) // check if we can configure device
      {
        switch (device_cap) {
        case V4L2_CAP_VIDEO_CAPTURE:
          dev->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
          break;
        case V4L2_CAP_VIDEO_OUTPUT:
          dev->buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
          break;
        default: // unimplemented
          fprintf(stderr, "UNSUPPORTED\n");
          exit(EXIT_FAILURE);
          break;
        }
      }
    }
  }
  if (dev->buf_type) {
    // Kernel rejects format with empty type. How do we know type?
    // If we know the device caps, we will also know supported
    // req types
    dev->format.type = dev->buf_type;
  }
  // Format negotitation -> REQBUF -> QUERYBUF -> mmap() -> QBUF ->
  // VIDIOC_STREAMON 2-4 buffers each for capture and output, at least 2 buffer
  // to stop hardware stalls
  printf("%s: S_FMT\n", dev->name);
  if (dev->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
    dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    dev->format.fmt.pix.width = width;
    dev->format.fmt.pix.height = height;
    if (-1 == xioctl(dev->fd, VIDIOC_S_FMT, &dev->format)) {
      errno_exit("VIDIOC_S_FMT");
    }

    struct v4l2_streamparm fps = {0};
    fps.type = dev->buf_type;
    fps.parm.capture.timeperframe.numerator = 1;
    fps.parm.capture.timeperframe.denominator = 5; // 5 fps
    if (-1 == xioctl(dev->fd, VIDIOC_S_PARM, &fps)) {
      errno_exit("VIDIOC_S_PARM");
    }
  } else if (dev->format.type == V4L2_CAP_VIDEO_OUTPUT) {
    dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    dev->format.fmt.pix.width = width;
    dev->format.fmt.pix.height = height;
    dev->format.fmt.pix.bytesperline = width * YUYV_BYTES_PER_PIXEL;
    dev->format.fmt.pix.sizeimage = width * height * YUYV_BYTES_PER_PIXEL;
    if (-1 == xioctl(dev->fd, VIDIOC_S_FMT, &dev->format)) {
      errno_exit("VIDIOC_S_FMT");
    }
  } else {
    fprintf(stderr, "UNSUPPORTED\n");
    exit(EXIT_FAILURE);
  }
  mmap_buf(4, dev);
  printf("%s: STREAMON\n", dev->name);
  start_stream(dev);
}

void deinit_device(struct device *device) {
  // VIDIOC_STREAMOFF -> mumap() -> buffer free -> close device
  printf("%s: STREAMOFF\n", device->name);
  stop_stream(device);
  printf("%s: MUMAP\n", device->name);
  munmap_buf(device);
  printf("%s: CLOSE\n", device->name);
  close(device->fd);
}

void req_buf(struct device *dev) {
  struct v4l2_requestbuffers req = {0};
  req.count = dev->buffer_count;
  req.type = dev->buf_type;
  req.memory = dev->mem_type;
  if (-1 == xioctl(dev->fd, VIDIOC_REQBUFS, &req)) {
    errno_exit("VIDIOC_REQBUFS");
  }
  dev->buffer_count = req.count; // May get less than requested
}

void mmap_buf(int count, struct device *dev) {
  dev->mem_type = V4L2_MEMORY_MMAP;
  dev->buffer_count = count; // Each REQBUF call gives you count buffers, not
                             // increment/decrement
  printf("%s: REQBUF\n", dev->name);
  req_buf(dev);
  if (dev->buffer_count < 2) {
    fprintf(stderr, "Out of memory on device\n");
    exit(EXIT_FAILURE);
  }
  dev->buffer = calloc(dev->buffer_count, sizeof(struct buffer));
  if (!dev->buffer) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  printf("%s: MMAP\n", dev->name);
  for (int i = 0; i < dev->buffer_count; ++i) {
    struct v4l2_buffer buf = {0};
    buf.index = i;
    buf.type = dev->buf_type;
    buf.memory = dev->mem_type;
    if (-1 == xioctl(dev->fd, VIDIOC_QUERYBUF, &buf)) {
      errno_exit("VIDIOC_QUERYBUF");
    }
    dev->buffer[i].length = buf.length;
    dev->buffer[i].start =
        mmap(NULL, buf.length, PROT_READ | PROT_WRITE /* required */,
             MAP_SHARED /* recommended */, dev->fd, buf.m.offset);
    if (dev->buffer[i].start == MAP_FAILED) {
      errno_exit("mmap");
    }
  }
}

void munmap_buf(struct device *dev) {
  for (int i = 0; i < dev->buffer_count; ++i) {
    if (-1 == munmap(dev->buffer[i].start, dev->buffer[i].length)) {
      errno_exit("munmap");
    }
    dev->buffer[i].length = 0;
    dev->buffer[i].start = NULL;
  }
  free(dev->buffer);
  dev->buffer_count = 0; // See mmap() note on count
  req_buf(dev);
}

void start_stream(struct device *dev) {
  for (int i = 0; i < dev->buffer_count; ++i) {
    struct v4l2_buffer buf = {0};
    buf.index = i;
    buf.memory = dev->mem_type;
    buf.type = dev->buf_type;
    if (-1 == xioctl(dev->fd, VIDIOC_QBUF, &buf)) {
      errno_exit("VIDIOC_QBUF");
    }
  }
  if (-1 == xioctl(dev->fd, VIDIOC_STREAMON, &dev->buf_type)) {
    errno_exit("VIDIOC_STREAMON");
  }
}

void stop_stream(struct device *dev) {
  if (-1 == xioctl(dev->fd, VIDIOC_STREAMOFF, &dev->buf_type)) {
    errno_exit("VIDIOC_STREAMON");
  }
}

void enum_caps(struct device *dev) {
  // Upper idx not known - go up until ioctl call fails
  // Hiearchical structure pix format -> frame size -> interval
  for (int i = 0;; ++i) {
    struct v4l2_fmtdesc fmt = {0};
    fmt.index = i;
    fmt.type = dev->buf_type;
    if (-1 == xioctl(dev->fd, VIDIOC_ENUM_FMT, &fmt)) {
      if (errno == EINVAL)
        break;
      errno_exit("VIDIOC_ENUM_FRAMESIZES");
    }
    printf("pixel fmt %d\n", i);
    for (int j = 0;; ++j) {
      struct v4l2_frmsizeenum frame_size = {0};
      frame_size.index = j;
      frame_size.pixel_format = fmt.pixelformat;
      if (-1 == xioctl(dev->fd, VIDIOC_ENUM_FRAMESIZES, &frame_size)) {
        if (errno == EINVAL)
          break;
        errno_exit("VIDIOC_ENUM_FRAMESIZES");
      }
      printf("frame size %d\n", j);
      for (int k = 0;; ++k) {
        struct v4l2_frmivalenum interval = {0};
        interval.index = k;
        interval.pixel_format = fmt.pixelformat;
        if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
          interval.width = frame_size.discrete.width;
          interval.height = frame_size.discrete.height;
        } else {
          fprintf(stderr, "Unsupported frame type\n");
          // exit(EXIT_FAILURE);
        }
        if (-1 == xioctl(dev->fd, VIDIOC_ENUM_FRAMEINTERVALS, &interval)) {
          if (errno == EINVAL)
            break;
          errno_exit("VIDIOC_ENUM_FRAMEINTERVALS");
        }
        printf("interval %d\n", k);
      }
    }
  }
}
