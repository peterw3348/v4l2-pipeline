#pragma once
#include <stddef.h> // size_t
#include <stdint.h> // uint8_t

struct buffer {
  uint8_t *start;
  size_t length;
};
