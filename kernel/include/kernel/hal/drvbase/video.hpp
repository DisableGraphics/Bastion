#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/kernel/timeconst.hpp>
#include <stdint.h>

namespace hal {
	struct color {
		int r, g, b, a;
	};
	class VideoDriver : public hal::Driver {
		public:
			VideoDriver(uint8_t* framebuffer,
				uint32_t width,
				uint32_t height,
				uint32_t pitch,
				uint32_t depth);
			void init(tc::timertime interval);
			virtual bool is_text_only() = 0;
			
			virtual void draw_char(char c, int x, int y) = 0;
			virtual void draw_string(char* str, int x, int y) = 0;
			virtual void draw_pixel(int x, int y, color c) = 0;
			virtual void draw_rectangle(int x, int y, int w, int h, color c) = 0;
			virtual void clear() = 0;

			/// Copy backbuffer into framebuffer if backbuffer is dirty
			virtual void copy();
		protected:
			bool dirty = false;
			uint8_t* framebuffer;
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			uint32_t depth;
			const uint32_t scrsize;
			uint8_t* backbuffer;
	};
}