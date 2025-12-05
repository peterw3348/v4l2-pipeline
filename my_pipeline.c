#include "conversion.h"
#include "v4l2_helper.h"
#include <fcntl.h>
#include <linux/videodev2.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#define FRAME_DUMP_COUNT 10

/**
 * @file my_pipeline.c
 * @brief Example pipelines:
 *
 *   1. Capture-only mode:
 *        ./pipeline /dev/video0
 *      Saves FRAME_DUMP_COUNT JPEG frames to frames/frameX.jpg
 *
 *   2. Capture → Convert → Output mode:
 *        ./pipeline /dev/video0 /dev/video2
 *      Converts MJPEG → YUYV and streams into v4l2loopback.
 */

volatile sig_atomic_t running = 1;
void handle_sigint(int sig) { running = 0; }

/**
 * @brief Capture N JPEG frames and dump them to disk.
 */
void capture_frames(struct device *capture_device) {

  char filename[64];

  for (int i = 0; i < FRAME_DUMP_COUNT; ++i) {
    struct v4l2_buffer buf = {0};
    buf.type = capture_device->buf_type;
    buf.memory = capture_device->mem_type;

    printf("frame%d started\n", i);

    // Wait for dequeue
    while (running) {
      if (-1 == xioctl(capture_device->fd, VIDIOC_DQBUF, &buf)) {
        if (errno == EAGAIN)
          continue;
        errno_exit("VIDIOC_DQBUF");
      }
      break;
    }

    // Write JPEG to disk
    snprintf(filename, sizeof(filename), "frames/frame%d.jpg", i);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, capture_device->buffer[buf.index].start,
          capture_device->buffer[buf.index].length);
    close(fd);

    if (!running)
      break;

    // Requeue buffer
    if (-1 == xioctl(capture_device->fd, VIDIOC_QBUF, &buf))
      errno_exit("VIDIOC_QBUF");

    printf("frame%d done\n", i);
  }
}

/**
 * @brief Capture MJPEG frames, convert to YUYV, and feed an output device.
 */
void capture_to_output(struct device *capture_device,
                       struct device *output_device) {

  conversion_init();

  int i = 0;
  while (running) {
    printf("%d\n", i++);

    // Dequeue capture buffer
    struct v4l2_buffer cap_buf = {0};
    cap_buf.type = capture_device->buf_type;
    cap_buf.memory = capture_device->mem_type;

    while (1) {
      if (-1 == xioctl(capture_device->fd, VIDIOC_DQBUF, &cap_buf)) {
        if (errno == EAGAIN)
          continue;
        errno_exit("VIDIOC_DQBUF");
      }
      break;
    }

    // Dequeue output buffer
    struct v4l2_buffer out_buf = {0};
    out_buf.type = output_device->buf_type;
    out_buf.memory = output_device->mem_type;

    while (1) {
      if (-1 == xioctl(output_device->fd, VIDIOC_DQBUF, &out_buf)) {
        if (errno == EAGAIN)
          continue;
        errno_exit("VIDIOC_DQBUF");
      }
      break;
    }

    // Perform MJPEG → YUYV conversion
    jpeg_to_yuyv(capture_device->buffer[cap_buf.index],
                 output_device->buffer[out_buf.index]);

    // Required: bytesused must be set for output device
    out_buf.bytesused = output_device->format.fmt.pix.sizeimage;

    // Requeue both buffers
    if (-1 == xioctl(capture_device->fd, VIDIOC_QBUF, &cap_buf))
      errno_exit("VIDIOC_QBUF");

    if (-1 == xioctl(output_device->fd, VIDIOC_QBUF, &out_buf))
      errno_exit("VIDIOC_QBUF");
  }

  conversion_deinit();
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "%s <capture_device> [output_device]\n", argv[0]);
    return -1;
  }

  signal(SIGINT, handle_sigint);

  struct device capture_device = {0};
  struct device output_device = {0};

  int width = 160;
  int height = 120;

  // Initialize capture device
  init_device(argv[1], V4L2_CAP_VIDEO_CAPTURE, width, height, &capture_device);

  // With output target
  if (argc == 3) {
    init_device(argv[2], V4L2_CAP_VIDEO_OUTPUT, width, height, &output_device);
    capture_to_output(&capture_device, &output_device);
    deinit_device(&output_device);
  }
  // Capture-only
  else {
    capture_frames(&capture_device);
  }

  deinit_device(&capture_device);
  return 0;
}
