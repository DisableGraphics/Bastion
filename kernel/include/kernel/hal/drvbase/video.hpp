#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <stdint.h>

namespace hal {
	class VideoDriver : public hal::Driver {
		public:
			virtual bool is_text_only() = 0;
		protected:
			uint8_t* framebuffer;
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			uint32_t depth;
	};
}