#include "conversion.h"
#include <stdio.h>
#include <stdlib.h>
#include <turbojpeg.h>

static uint8_t *temp_i420 = NULL;
static size_t temp_i420_size = 0;
static int initialized = 0;

static int frame_width = 0;
static int frame_height = 0;
static int frame_subsamp = 0;

static tjhandle tj = NULL;

void conversion_init() {
  tj = tjInitDecompress();
  if (!tj) {
    fprintf(stderr, "tjInitDecompress failed: %s\n", tjGetErrorStr());
    exit(EXIT_FAILURE);
  }
}

void conversion_deinit() {
  if (temp_i420) {
    free(temp_i420);
    temp_i420 = NULL;
  }

  if (tj) {
    tjDestroy(tj);
    tj = NULL;
  }
}

static int decode_mjpeg_to_i420(const uint8_t *jpeg_buf,
                                unsigned long jpeg_size) {
  int width, height, subsamp, colorspace;

  if (tjDecompressHeader3(tj, jpeg_buf, jpeg_size, &width, &height, &subsamp,
                          &colorspace) < 0) {
    fprintf(stderr, "Header decode error: %s\n", tjGetErrorStr());
    return -1;
  }

  if (!initialized) {
    frame_width = width;
    frame_height = height;
    frame_subsamp = subsamp;

    temp_i420_size = tjBufSizeYUV2(width, 1, height, subsamp);
    temp_i420 = malloc(temp_i420_size);
    if (!temp_i420) {
      fprintf(stderr, "Failed to allocate I420 buffer\n");
      return -1;
    }

    initialized = 1;
  }

  if (width != frame_width || height != frame_height) {
    fprintf(stderr, "ERROR: Resolution changed mid-stream (%dx%d → %dx%d)\n",
            frame_width, frame_height, width, height);
    return -1;
  }

  if (tjDecompressToYUV2(tj, jpeg_buf, jpeg_size, temp_i420, width,
                         1, // 1-byte alignment
                         height, TJFLAG_FASTUPSAMPLE | TJFLAG_FASTDCT) < 0) {
    fprintf(stderr, "DecompressToYUV error: %s\n", tjGetErrorStr());
    return -1;
  }

  return 0;
}

static void i420_to_yuyv(uint8_t *dst) {

  const uint8_t *src_y = temp_i420;
  const uint8_t *src_u = temp_i420 + frame_width * frame_height;
  const uint8_t *src_v = src_u + (frame_width * frame_height) / 4;

  int width = frame_width;
  int height = frame_height;

  for (int y = 0; y < height; y++) {
    const uint8_t *row_y = src_y + y * width;
    const uint8_t *row_u = src_u + (y / 2) * (width / 2);
    const uint8_t *row_v = src_v + (y / 2) * (width / 2);

    uint8_t *row_dst = dst + y * width * 2;

    for (int x = 0; x < width; x += 2) {
      uint8_t Y0 = row_y[x];
      uint8_t Y1 = row_y[x + 1];

      uint8_t U0 = row_u[x / 2];
      uint8_t V0 = row_v[x / 2];

      row_dst[0] = Y0;
      row_dst[1] = U0;
      row_dst[2] = Y1;
      row_dst[3] = V0;

      row_dst += 4;
    }
  }
}

void jpeg_to_yuyv(struct buffer cap_buf, struct buffer out_buf) {
  if (!tj) {
    fprintf(stderr, "ERROR: conversion_init() was never called!\n");
    return;
  }

  if (decode_mjpeg_to_i420(cap_buf.start, cap_buf.length) < 0)
    return;

  i420_to_yuyv(out_buf.start);
}
