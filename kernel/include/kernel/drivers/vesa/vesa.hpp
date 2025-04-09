#pragma once
#include <kernel/hal/drvbase/video.hpp>

class VESADriver final : public hal::VideoDriver {
	public:
		VESADriver(uint8_t* framebuffer,
			uint32_t width,
			uint32_t height,
			uint32_t pitch,
			uint32_t depth,
			uint32_t red_pos,
			uint32_t green_pos,
			uint32_t blue_pos,
			uint32_t red_size,
			uint32_t green_size,
			uint32_t blue_size);
		void init() override;
		void handle_interrupt() override;
		bool is_text_only() override;
		void set_fonts(uint8_t* fontsarr);
		inline void draw_char(unsigned c, int x, int y) override;
		inline void draw_pixel(int x, int y, hal::color c) override;
		inline void draw_pixels(int x1, int y1, int w, int h, uint8_t* array) override;
		inline void draw_rectangle(int x1, int y1, int x2, int y2, hal::color c) override;
		void clear(hal::color c) override;
	private:
		inline uint8_t squish8_to_size(int val, uint8_t destsize);
		void set_blocks_as_dirty(int x1, int y1, int x2, int y2);

		const uint32_t red_pos;
		const uint32_t blue_pos;
		const uint32_t green_pos;
		const uint32_t red_size;
		const uint32_t blue_size;
		const uint32_t green_size;

		uint8_t depth_disp;
};