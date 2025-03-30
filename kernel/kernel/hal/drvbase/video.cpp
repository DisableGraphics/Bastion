#include <kernel/hal/drvbase/video.hpp>
#include <kernel/hal/managers/timermanager.hpp>
#include <string.h>

hal::VideoDriver::VideoDriver(uint8_t* framebuffer,
	uint32_t width,
	uint32_t height,
	uint32_t pitch,
	uint32_t depth,
	uint16_t freq) {
	
	// Ask the timermanager to call copy() each 1/freq interval
	//hal::TimerManager::get().exec_at(uint32_t ms, void (*fn)(volatile void *), volatile void *args)
}

void hal::VideoDriver::copy() {
	if(dirty)
		memcpy(framebuffer, backbuffer, height*pitch*depth);
}