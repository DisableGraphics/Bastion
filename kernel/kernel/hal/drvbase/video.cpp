#include <kernel/hal/drvbase/video.hpp>
#include <kernel/hal/managers/timermanager.hpp>
#include <kernel/kernel/log.hpp>
#include <string.h>

hal::VideoDriver::VideoDriver(uint8_t* framebuffer,
	uint32_t width,
	uint32_t height,
	uint32_t pitch,
	uint32_t depth) : 
	framebuffer(framebuffer),
	width(width),
	height(height),
	pitch(pitch),
	tiles_x((width + TILE_SIZE - 1) >> TILE_SIZE_DISP),
	tiles_y((height + TILE_SIZE - 1) >> TILE_SIZE_DISP),
	scrsize(height*pitch*(depth>>3)),
	ntiles(tiles_x*tiles_y) {
	row_pointers = reinterpret_cast<size_t*>(kcalloc(height, sizeof(size_t)));
	backbuffer = reinterpret_cast<uint8_t*>(kcalloc(scrsize, sizeof(*backbuffer)));
	dirty_tiles = reinterpret_cast<bool*>(kcalloc(ntiles, sizeof(bool)));
	dirty_tiles_for_clear = reinterpret_cast<bool*>(kcalloc(ntiles, sizeof(bool)));
}

void hal::VideoDriver::init(tc::timertime interval) {

}

inline void hal::VideoDriver::flush() {
	if(dirty) {
		for (size_t y = 0; y < tiles_y; y++) {
			const size_t row_offset = y * tiles_x;
			int y0 = y << TILE_SIZE_DISP;
			for (size_t x = 0; x < tiles_x; x++) {
				const size_t index = row_offset + x;
				if (!dirty_tiles[index])
					continue;
				int x0 = x << TILE_SIZE_DISP;
				for (int ty = 0; ty < TILE_SIZE; ty++) {
					const size_t offset = (y0 + ty) * pitch + (x0 << depth_disp);
					uint8_t* srcrow = backbuffer + offset;
					uint8_t* dstrow = framebuffer + offset;
					sse2_memcpy(dstrow, srcrow, TILE_SIZE << depth_disp);
				}
				dirty_tiles[index] = false;
			}
		}
		dirty = false;
	}
}