#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * @file buffer.h
 * @brief Simple struct representing a V4L2 memory-mapped buffer.
 *
 * This abstraction is used consistently across the project for both
 * capture and output devices. Each buffer consists of:
 *   - start  : pointer to mmap'ed memory
 *   - length : size of the mapped region
 */
struct buffer {
  uint8_t *start; ///< Pointer to buffer memory
  size_t length;  ///< Length of buffer in bytes
};
