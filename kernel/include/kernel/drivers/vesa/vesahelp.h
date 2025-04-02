#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

void fast_clear(uint32_t color, uint8_t* fb, size_t pixel_count);
void draw_rectangle(int x1, int y1, int x2, int y2, uint32_t packed_color, uint8_t* backbuffer, size_t* row_pointers, uint32_t depth_disp);
void draw_pixels(int x1, int y1, int x2, int y2, uint8_t* pixels, uint8_t* backbuffer, size_t* row_pointers, uint32_t depth_disp);

#ifdef __cplusplus
}
#endif