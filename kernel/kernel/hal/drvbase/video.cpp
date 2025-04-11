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
}

void hal::VideoDriver::init(tc::timertime interval) {

}

inline void hal::VideoDriver::flush() {
	if(dirty) {
		for(size_t i = 0; i < ntiles; i++) {
			if(dirty_tiles[i]) {
				sse2_memcpy(framebuffer + (i << DISP_BLOCK_SIZE), backbuffer + (i << DISP_BLOCK_SIZE), BLOCK_SIZE);
				dirty_tiles[i] = false;
			}
		}
		dirty = false;
	}
}