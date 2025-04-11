#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/kernel/timeconst.hpp>
#include <stdint.h>

// Shift length of a block size (for optimizations)
constexpr uint32_t DISP_BLOCK_SIZE = 14;
constexpr uint32_t BLOCK_SIZE = (1 << DISP_BLOCK_SIZE);

// Tiles are 64x64 pixels
constexpr uint32_t TILE_SIZE_DISP = 6;
constexpr uint32_t TILE_SIZE = (1 << TILE_SIZE_DISP);

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

			inline virtual uint8_t get_font_width() = 0;
			inline virtual uint8_t get_font_height() = 0;
			inline virtual uint32_t get_width() { return width; }
			inline virtual uint32_t get_height() { return height; }
			inline virtual uint32_t get_pitch() { return pitch; }
			inline virtual uint32_t get_depth() { return depth; }

			/// Copy backbuffer into framebuffer if backbuffer is dirty
			virtual void flush();
		protected:
			bool dirty = false;
			bool* dirty_tiles;
			bool* dirty_tiles_for_clear;
			uint8_t* framebuffer;
			uint32_t width;
			uint32_t height;
			uint32_t pitch;
			uint32_t depth;
			const uint32_t tiles_x, tiles_y;
			// Desperate times call for desperate measures
			size_t* row_pointers;
			const uint32_t scrsize, ntiles;
			uint8_t* backbuffer;
			uint8_t depth_disp;
	};
}