#pragma once
#include "buffer.h"
#include <stdint.h>

/**
 * @file conversion.h
 * @brief MJPEG → YUYV conversion API using libjpeg-turbo.
 *
 * These functions provide a small userspace conversion pipeline:
 *   1. Decode MJPEG into an intermediate RGB888 buffer.
 *   2. Convert RBG888 into YUYV (YUY2) for V4L2 output devices.
 *
 * The conversion state is kept internally and must be initialized
 * with conversion_init() before calling jpeg_to_yuyv().
 */

/**
 * @brief Initialize libjpeg-turbo decoder and internal buffers.
 *
 * Must be called once before jpeg_to_yuyv().
 */
void conversion_init();

/**
 * @brief Free internal conversion buffers and destroy decoder instance.
 *
 * Safe to call once no more conversions are needed.
 */
void conversion_deinit();

/**
 * @brief Convert a captured MJPEG frame to a YUYV buffer.
 *
 * @param cap_buf  A V4L2 buffer containing the MJPEG-encoded frame.
 * @param out_buf  A V4L2 buffer (sizeimage bytes) to receive YUYV pixels.
 *
 * The output buffer must be sized according to:
 *     width * height * 2 (for YUYV 4:2:2)
 */
void jpeg_to_yuyv(struct buffer cap_buf, struct buffer out_buf);
