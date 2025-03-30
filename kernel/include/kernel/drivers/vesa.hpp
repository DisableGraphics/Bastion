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
		void draw_char(char c, int x, int y) override;
		void draw_string(char* str, int x, int y) override;
		void draw_pixel(int x, int y, hal::color c) override;
		void draw_rectangle(int x, int y, int w, int h, hal::color c) override;
		void clear() override;
	private:
		typedef void (VESADriver::*draw_fn)(uint8_t* where, hal::color c);
		draw_fn draw_raw = nullptr;

		void draw_32(uint8_t* where, hal::color c);
		void draw_24(uint8_t* where, hal::color c);
		void draw_16(uint8_t* where, hal::color c);

		inline uint8_t squish8_to_size(int val, uint8_t destsize);

		uint32_t red_pos;
		uint32_t blue_pos;
		uint32_t green_pos;
		uint32_t red_size;
		uint32_t blue_size;
		uint32_t green_size;
};