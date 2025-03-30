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
	scrsize(height*pitch*(depth/8)),
	nblocks(scrsize/BLOCK_SIZE) {
	backbuffer = reinterpret_cast<uint8_t*>(kcalloc(scrsize, sizeof(*backbuffer)));
	dirty_blocks = reinterpret_cast<bool*>(kcalloc(nblocks, sizeof(bool)));
}

void hal::VideoDriver::init(tc::timertime interval) {

}

void hal::VideoDriver::flush() {
	if(dirty) {
		for(size_t i = 0; i < nblocks; i++) {
			if(dirty_blocks[i]) {
				memcpar(framebuffer + (i << DISP_BLOCK_SIZE), backbuffer + (i << DISP_BLOCK_SIZE), BLOCK_SIZE);
				dirty_blocks[i] = false;
			}
		}
		dirty = false;
	}
}