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
  if (-1 == xioctl(capture_device->fd, VIDIOC_G_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_G_FMT");
  }
  assert(capture_device->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE);
  assert(capture_device->format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG);

  capture_device->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
  capture_device->format.fmt.pix.width = 160;
  capture_device->format.fmt.pix.height = 120;
  if (-1 == xioctl(capture_device->fd, VIDIOC_S_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_S_FMT");
  }
  struct v4l2_streamparm fps = {0};
  fps.type = capture_device->buf_type;
  fps.parm.capture.timeperframe.numerator = 1;
  fps.parm.capture.timeperframe.denominator = 5;
  if (-1 == xioctl(capture_device->fd, VIDIOC_S_PARM, &fps)) {
    errno_exit("VIDIOC_S_PARM");
  }
  printf("format set on capture device\n");

  if (-1 == xioctl(capture_device->fd, VIDIOC_G_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_G_FMT");
  }

  mmap_buf(4, capture_device);
  printf("capture buffers requested\n");

  start_stream(capture_device);
  printf("capture stream started\n");

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

  stop_stream(capture_device);
  printf("capture stream stopped\n");

  munmap_buf(capture_device);
  printf("capture buffers freed\n");

  close(capture_device->fd);
}

void capture_to_output(struct device *capture_device,
                       struct device *output_device) {
  //  Negotiating formats between devices
  capture_device->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
  capture_device->format.fmt.pix.width = 160;
  capture_device->format.fmt.pix.height = 120;
  if (-1 == xioctl(capture_device->fd, VIDIOC_S_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_S_FMT");
  }
  struct v4l2_streamparm fps = {0};
  fps.type = capture_device->buf_type;
  fps.parm.capture.timeperframe.numerator = 1;
  fps.parm.capture.timeperframe.denominator = 5;
  if (-1 == xioctl(capture_device->fd, VIDIOC_S_PARM, &fps)) {
    errno_exit("VIDIOC_S_PARM");
  }
  printf("format set on capture device\n");

  output_device->format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  output_device->format.fmt.pix.width = 160;
  output_device->format.fmt.pix.height = 120;
  output_device->format.fmt.pix.bytesperline = 160 * 2;
  output_device->format.fmt.pix.sizeimage = 160 * 120 * 2;
  if (-1 == xioctl(output_device->fd, VIDIOC_S_FMT, &output_device->format)) {
    errno_exit("VIDIOC_S_FMT");
  }
  printf("format set on output device\n");

  // Buffer negotiation -> mmap() -> VIDIOC_STREAMON
  // 2-4 buffers each for capture and output, at least 2 to stop hardware stalls
  // Size decided by format negotiation above e.g. 1280/720 'MJPG' ~ 1.2 MB

  mmap_buf(4, capture_device);
  printf("capture buffers requested\n");

  mmap_buf(4, output_device);
  printf("output buffers requested\n");

  start_stream(capture_device);
  printf("capture stream started\n");
  start_stream(output_device);
  printf("output stream started\n");

  conversion_init();
  int i = 0;
  while (running) {
    printf("%d:\n", i);
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
    printf("frame exit cap\n");
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
    printf("frame exit out\n");
    jpeg_to_yuyv(capture_device->buffer[cap_buf.index],
                 output_device->buffer[out_buf.index]);
    out_buf.bytesused = output_device->format.fmt.pix.sizeimage;

    if (-1 == xioctl(capture_device->fd, VIDIOC_QBUF, &cap_buf)) {
      errno_exit("VIDIOC_QBUF cap");
    }
    if (-1 == xioctl(output_device->fd, VIDIOC_QBUF, &out_buf)) {
      errno_exit("VIDIOC_QBUF out");
    }
  }

  conversion_deinit();

  stop_stream(capture_device);
  printf("capture stream stopped\n");
  stop_stream(output_device);
  printf("output stream stopped\n");

  // Cleanup opposite direction as above
  // VIDIOC_STREAMOFF -> mumap() -> buffer free -> close device
  munmap_buf(capture_device);
  printf("capture buffers freed\n");

  munmap_buf(output_device);
  printf("output buffers freed\n");

  close(capture_device->fd);
  close(output_device->fd);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "%s <capture device> <output_device>\n", argv[0]);
    return -1;
  }
  signal(SIGINT, handle_sigint);

  struct device capture_device = {0};
  struct device output_device = {0};
  int status;
  if ((status = init_device(argv[1], V4L2_CAP_VIDEO_CAPTURE,
                            &capture_device)) != INIT_SUCCESS) {
    init_err(status);
    return -1;
  }
  if ((status = init_device(argv[2], V4L2_CAP_VIDEO_OUTPUT, &output_device)) !=
      INIT_SUCCESS) {
    init_err(status);
    return -1;
  }

  // printf("enum_caps for %d\n", capture_device.fd);
  // enum_caps(&capture_device);
  // printf("enum_caps for %d\n", output_device.fd);
  // enum_caps(&output_device);

  // capture_frames(&capture_device);
  capture_to_output(&capture_device, &output_device);
  return 0;
}
