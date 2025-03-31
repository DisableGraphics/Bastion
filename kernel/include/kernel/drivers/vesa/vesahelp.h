#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

void fast_clear(uint32_t color, uint8_t* fb, size_t pixel_count);

#ifdef __cplusplus
}
#endif