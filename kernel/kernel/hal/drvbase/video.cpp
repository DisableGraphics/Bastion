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
	scrsize(height*pitch*(depth/8)) {
	backbuffer = reinterpret_cast<uint8_t*>(kcalloc(scrsize, sizeof(*backbuffer)));
}

void hal::VideoDriver::init(tc::timertime interval) {
	volatile void** args = new volatile void*[3]{this, nullptr, nullptr};
	void (*lambda)(volatile void*) = [](volatile void* args) {
		void** argsn = reinterpret_cast<void**>(const_cast<void*>(reinterpret_cast<volatile void*>(args)));
		log(INFO, "args[0] = %p, args[1] = %p, args[2] = %p", argsn);
		tc::timertime interval = reinterpret_cast<tc::timertime>(argsn[2]);
		void (*lambda)(volatile void*) = reinterpret_cast<void(*)(volatile void*)>(argsn[1]);
		hal::VideoDriver* self = reinterpret_cast<hal::VideoDriver*>(argsn[0]);
		self->copy();
		hal::TimerManager::get().exec_at(interval, lambda, args);
	};
	args[0] = this;
	args[1] = reinterpret_cast<void*>(lambda);
	args[2] = reinterpret_cast<void*>(interval);

	// Ask the timermanager to call copy() each interval to refresh the screen
	hal::TimerManager::get()
		.exec_at(interval, lambda, args);
}

void hal::VideoDriver::copy() {
	log(INFO, "Framebuffer: %p, backbuffer: %p", framebuffer, backbuffer);
	if(dirty) {
		memcpy(framebuffer, backbuffer, scrsize);
		dirty = false;
	}
}