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

		inline uint8_t get_font_width() override;
		inline uint8_t get_font_height() override;
		inline uint32_t get_width() override;
		inline uint32_t get_height() override;
		inline uint32_t get_pitch() override;
		inline uint32_t get_depth() override;
		
		void clear(hal::color c) override;
	private:
		inline uint8_t squish8_to_size(int val, uint8_t destsize);
		void mark_rectangle_as_dirty(int x1, int y1, int x2, int y2);
		inline void mark_tile_as_dirty(int x, int y) {
			int tile_x = x >> TILE_SIZE_DISP;
			int tile_y = y >> TILE_SIZE_DISP;
			const size_t pos = (tile_y * tiles_x) + tile_x;
			dirty_tiles[pos] = true;
			dirty_tiles_for_clear[pos] = true;
		};

		const uint32_t red_pos;
		const uint32_t green_pos;
		const uint32_t blue_pos;
		const uint32_t red_size;
		const uint32_t green_size;
		const uint32_t blue_size;
		hal::color prevclearcolor;
};