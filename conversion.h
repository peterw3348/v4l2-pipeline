#pragma once
#include "buffer.h"
#include <stdint.h>

void conversion_init();

void conversion_deinit();

void jpeg_to_yuyv(struct buffer cap_buf, struct buffer out_buf);
