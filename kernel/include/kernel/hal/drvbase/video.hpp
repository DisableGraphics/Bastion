#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <stdint.h>

namespace hal {
	class VideoDriver : public hal::Driver {
		public:
			VideoDriver(uint8_t* framebuffer,
				uint32_t width,
				uint32_t height,
				uint32_t pitch,
				uint32_t depth,
				uint16_t freq);
			virtual bool is_text_only() = 0;

			/// Copy backbuffer into framebuffer if backbuffer is dirty
			virtual void copy();
		protected:
			bool dirty = false;
			uint8_t* framebuffer;
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			uint32_t depth;
			uint8_t* backbuffer;
	};
}