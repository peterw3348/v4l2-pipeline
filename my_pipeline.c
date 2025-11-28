#include <assert.h>
#include <fcntl.h> // for open()
#include <stdio.h>
#include <sys/ioctl.h> // for ioctl()
#include <unistd.h>    // for close()

#include <linux/videodev2.h>

#include "v4l2_helper.h"

// V4L2 demo using integrated webcam, v4l2 loopback and mmap()

void capture_frames(struct device *capture_device) {
  if (-1 == ioctl(capture_device->fd, VIDIOC_G_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_G_FMT");
  }
  assert(capture_device->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE);
  assert(capture_device->format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG);

  if (-1 == ioctl(capture_device->fd, VIDIOC_G_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_G_FMT");
  }

  mmap_buf(4, capture_device);
  printf("capture buffers requested\n");

  start_stream(capture_device);
  printf("capture stream started\n");
  
  stop_stream(capture_device);
  printf("capture stream stopped\n");

  munmap_buf(capture_device);
  printf("capture buffers freed\n");

  close(capture_device->fd);
}

void capture_to_output(struct device *capture_device,
                       struct device *output_device) {
  //  Negotiating formats between devices
  if (-1 == ioctl(capture_device->fd, VIDIOC_G_FMT, &capture_device->format)) {
    errno_exit("VIDIOC_G_FMT");
  }
  output_device->format = capture_device->format;
  output_device->format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT; // Change type
  if (-1 == ioctl(output_device->fd, VIDIOC_S_FMT, &output_device->format)) {
    errno_exit("VIDIOC_S_FMT");
  }

  // Buffer negotiation -> mmap() -> VIDIOC_STREAMON
  // 2-4 buffers each for capture and output, at least 2 to stop hardware stalls
  // Size decided by format negotiation above e.g. 1280/720 'MJPG' ~ 1.2 MB

  mmap_buf(4, capture_device);
  printf("capture buffers requested\n");

  mmap_buf(4, output_device);
  printf("output buffers requested\n");

  // TODO: add STREAMON/STREAMOFF code for both devices
  // Add conversion from MJPEG to YUV

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
  capture_frames(&capture_device);
  // capture_to_output(&capture_device, &output_device);
  return 0;
}
