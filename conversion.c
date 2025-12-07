#include "conversion.h"
#include <stdio.h>
#include <stdlib.h>
#include <turbojpeg.h>

static tjhandle tj = NULL;
static uint8_t *rgb_buf = NULL;
static size_t rgb_buf_size = 0;

static int frame_width = 0;
static int frame_height = 0;
static int initialized = 0;

void conversion_init() {
  tj = tjInitDecompress();
  if (!tj) {
    fprintf(stderr, "tjInitDecompress failed: %s\n", tjGetErrorStr());
    exit(EXIT_FAILURE);
  }
}

void conversion_deinit() {
  if (rgb_buf)
    free(rgb_buf);
  if (tj)
    tjDestroy(tj);
  rgb_buf = NULL;
  tj = NULL;
  initialized = 0;
}

static int decode_mjpeg_to_rgb(const uint8_t *jpeg_buf,
                               unsigned long jpeg_size) {
  int width, height, subsamp, colorspace;

  if (tjDecompressHeader3(tj, jpeg_buf, jpeg_size, &width, &height, &subsamp,
                          &colorspace) < 0) {
    fprintf(stderr, "Header decode error: %s\n", tjGetErrorStr());
    return -1;
  }

  // Allocate RGB buffer once
  if (!initialized) {
    frame_width = width;
    frame_height = height;

    rgb_buf_size = width * height * 3; // RGB24
    rgb_buf = malloc(rgb_buf_size);
    if (!rgb_buf) {
      fprintf(stderr, "Failed to allocate rgb_buf\n");
      return -1;
    }

    initialized = 1;
  }

  // Safety check (webcam resolutions should not change)
  if (width != frame_width || height != frame_height) {
    fprintf(stderr, "Resolution changed unexpectedly\n");
    return -1;
  }

  // Decode to RGB (no chroma subsampling loss)
  if (tjDecompress2(tj, jpeg_buf, jpeg_size, rgb_buf, width, 0, height,
                    TJPF_RGB, TJFLAG_FASTUPSAMPLE | TJFLAG_FASTDCT) < 0) {
    fprintf(stderr, "RGB decode error: %s\n", tjGetErrorStr());
    return -1;
  }

  return 0;
}

static inline uint8_t clamp(int v) {
  if (v < 0)
    return 0;
  if (v > 255)
    return 255;
  return v;
}

static void rgb_to_yuyv(uint8_t *dst) {
  int width = frame_width;
  int height = frame_height;

  for (int y = 0; y < height; y++) {

    uint8_t *row_dst = dst + y * width * 2;
    uint8_t *row_src = rgb_buf + y * width * 3;

    for (int x = 0; x < width; x += 2) {

      // Pixel 0
      int r0 = row_src[0];
      int g0 = row_src[1];
      int b0 = row_src[2];

      // Pixel 1
      int r1 = row_src[3];
      int g1 = row_src[4];
      int b1 = row_src[5];

      // Compute Y0, Y1
      int Y0 = (77 * r0 + 150 * g0 + 29 * b0) >> 8;
      int Y1 = (77 * r1 + 150 * g1 + 29 * b1) >> 8;

      // Compute shared U, V (average both pixels)
      int avg_r = (r0 + r1) / 2;
      int avg_g = (g0 + g1) / 2;
      int avg_b = (b0 + b1) / 2;

      int U = ((-43 * avg_r - 85 * avg_g + 128 * avg_b) >> 8) + 128;
      int V = ((128 * avg_r - 107 * avg_g - 21 * avg_b) >> 8) + 128;

      row_dst[0] = clamp(Y0);
      row_dst[1] = clamp(U);
      row_dst[2] = clamp(Y1);
      row_dst[3] = clamp(V);

      row_dst += 4;
      row_src += 6; // two RGB pixels
    }
  }
}

void jpeg_to_yuyv(struct buffer cap_buf, struct buffer out_buf) {
  if (!tj) {
    fprintf(stderr, "conversion_init() not called\n");
    return;
  }

  if (decode_mjpeg_to_rgb(cap_buf.start, cap_buf.length) < 0)
    return;

  rgb_to_yuyv(out_buf.start);
}
