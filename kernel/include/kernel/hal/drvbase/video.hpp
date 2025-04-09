#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/kernel/timeconst.hpp>
#include <stdint.h>

// Shift length of a block size (for optimizations)
constexpr uint32_t DISP_BLOCK_SIZE = 12;
constexpr uint32_t BLOCK_SIZE = (1 << DISP_BLOCK_SIZE);

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
			
			inline virtual void draw_char(unsigned c, int x, int y) = 0;
			inline virtual void draw_pixel(int x, int y, color c) = 0;
			inline virtual void draw_pixels(int x1, int y1, int w, int h, uint8_t* array) = 0;
			inline virtual void draw_rectangle(int x1, int y1, int x2, int y2, color c) = 0;
			inline virtual void clear(color c) = 0;

			/// Copy backbuffer into framebuffer if backbuffer is dirty
			virtual void flush();
		protected:
			bool dirty = false;
			bool* dirty_blocks;
			uint8_t* framebuffer;
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			uint32_t depth;
			// Desperate times call for desperate measures
			size_t* row_pointers;
			const uint32_t scrsize, nblocks;
			uint8_t* backbuffer;
	};
}