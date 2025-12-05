#include "conversion.h"
#include <assert.h>
#include <fcntl.h>  // for open()
#include <signal.h> // for signal
#include <stdio.h>
#include <sys/ioctl.h> // for ioctl()
#include <unistd.h>    // for close()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

#define FRAME_DUMP_COUNT 10

volatile sig_atomic_t running = 1;

void handle_sigint(int sig) { running = 0; }

// V4L2 demo using integrated webcam, v4l2 loopback and mmap()

void capture_frames(struct device *capture_device) {
  char filename[64];
  for (int i = 0; i < FRAME_DUMP_COUNT; ++i) {
    struct v4l2_buffer buf = {0};
    buf.type = capture_device->buf_type;
    buf.memory = capture_device->mem_type;

    printf("frame%d started\n", i);
    while (running) {
      if (-1 == xioctl(capture_device->fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
          break;
        case EIO:
        default:
          errno_exit("VIDIOC_DQBUF");
        }
      } else {
        break;
      }
    }

    snprintf(filename, sizeof(filename), "frames/frame%d.jpg", i);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, capture_device->buffer[buf.index].start,
          capture_device->buffer[buf.index].length);
    close(fd);

    if (-1 == xioctl(capture_device->fd, VIDIOC_QBUF, &buf)) {
      errno_exit("VIDIOC_QBUF");
    }
    printf("frame%d done\n", i);
  }
}

void capture_to_output(struct device *capture_device,
                       struct device *output_device) {
  conversion_init();
  int i = 0;
  while (running) {
    printf("%d\n", i);
    ++i;
    struct v4l2_buffer cap_buf = {0};
    cap_buf.type = capture_device->buf_type;
    cap_buf.memory = capture_device->mem_type;
    while (1) {
      if (-1 == xioctl(capture_device->fd, VIDIOC_DQBUF, &cap_buf)) {
        switch (errno) {
        case EAGAIN:
          break;
        case EIO:
        default:
          errno_exit("VIDIOC_DQBUF");
        }
      } else {
        break;
      }
    }
    struct v4l2_buffer out_buf = {0};
    out_buf.type = output_device->buf_type;
    out_buf.memory = output_device->mem_type;
    while (1) {
      if (-1 == xioctl(output_device->fd, VIDIOC_DQBUF, &out_buf)) {
        switch (errno) {
        case EAGAIN:
          break;
        case EIO:
        default:
          errno_exit("VIDIOC_DQBUF");
        }
      } else {
        break;
      }
    }
    jpeg_to_yuyv(capture_device->buffer[cap_buf.index],
                 output_device->buffer[out_buf.index]);
    out_buf.bytesused = output_device->format.fmt.pix.sizeimage;

    if (-1 == xioctl(capture_device->fd, VIDIOC_QBUF, &cap_buf)) {
      errno_exit("VIDIOC_QBUF");
    }
    if (-1 == xioctl(output_device->fd, VIDIOC_QBUF, &out_buf)) {
      errno_exit("VIDIOC_QBUF");
    }
  }
  conversion_deinit();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "%s <capture device> [output_device]\n", argv[0]);
    return -1;
  }
  signal(SIGINT, handle_sigint);

  struct device capture_device = {0};
  struct device output_device = {0};
  int width = 160;
  int height = 120;

  init_device(argv[1], V4L2_CAP_VIDEO_CAPTURE, width, height, &capture_device);

  if (argc == 3) {
    init_device(argv[2], V4L2_CAP_VIDEO_OUTPUT, width, height, &output_device);
    capture_to_output(&capture_device, &output_device);
    deinit_device(&output_device);
  } else if (argc == 2) {
    capture_frames(&capture_device);
  }
  deinit_device(&capture_device);
  return 0;
}
