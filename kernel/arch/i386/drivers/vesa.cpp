#include <kernel/drivers/vesa.hpp>
#include <string.h>
#include <kernel/kernel/log.hpp>

VESADriver::VESADriver(uint8_t* framebuffer,
	uint32_t width,
	uint32_t height,
	uint32_t pitch,
	uint32_t depth,
	uint32_t red_pos,
	uint32_t green_pos,
	uint32_t blue_pos,
	uint32_t red_size,
	uint32_t green_size,
	uint32_t blue_size) : 
	hal::VideoDriver(framebuffer,
		width,
		height,
		pitch,
		depth),
		red_pos(red_pos),
		green_pos(green_pos),
		blue_pos(blue_pos),
		red_size(red_size),
		green_size(green_size),
		blue_size(blue_size)
{
	switch(depth) {
		case 32:
			blue_pos/=8;
			red_pos/=8;
			green_pos/=8;
			draw_raw = &VESADriver::draw_32;
			break;
		case 24:
			blue_pos/=8;
			red_pos/=8;
			green_pos/=8;
			draw_raw = &VESADriver::draw_24;
			break;
		case 16:
		case 15:
			draw_raw = &VESADriver::draw_16;
			break;
		default:
			// Shitty default
			draw_raw = &VESADriver::draw_24;
	}

	log(INFO, R"(VESA Framebuffer with:
	- framebuffer: %p,
	- size of framebuffer: %p,
	- width: %d,
	- height: %d,
	- pitch: %d,
	- depth: %d,
	- red_pos: %d,
	- green_pos: %d,
	- blue_pos: %d,
	- red_mask: %d,
	- green_mask: %d,
	- blue_mask: %d)",
	framebuffer,
	scrsize,
	width,
	height,
	pitch,
	depth,
	red_pos,
	green_pos,
	blue_pos,
	red_size,
	green_size,
	blue_size);
	log(INFO, "Back buffer: %p", backbuffer);

}

void VESADriver::init() {
	// Reasonable default of 60 Hz (16.667 ms)
	hal::VideoDriver::init(16667*tc::us);
}

void VESADriver::handle_interrupt() {
	// Do nothing. The interrupt is handled by the timer
}

bool VESADriver::is_text_only() {
	return false;
}

void VESADriver::draw_char(char c, int x, int y) {

}

void VESADriver::draw_string(char* str, int x, int y) {

}

void VESADriver::draw_pixel(int x, int y, hal::color c) {
	dirty = true;
	unsigned where = x*depth + y*pitch;
	(this->*draw_raw)(backbuffer+where, c);
}

void VESADriver::draw_rectangle(int x, int y, int w, int h, hal::color c) {

}

void VESADriver::clear() {
	memset(backbuffer, 0, scrsize);
}

void VESADriver::draw_32(uint8_t* where, hal::color c) {
	where[red_pos] = c.r;
	where[green_pos] = c.g;
	where[blue_pos] = c.b;
}

void VESADriver::draw_24(uint8_t* where, hal::color c) {
	where[red_pos] = c.r;
	where[green_pos] = c.g;
	where[blue_pos] = c.b;
}

void VESADriver::draw_16(uint8_t* where, hal::color c) {
	// Now this is where the fun part begins
	// Usually the format for these types of buffers
	// are 5 R 5 G 5 B but not always.
	hal::color squished {
		squish8_to_size(c.r, red_size),
		squish8_to_size(c.g, green_size),
		squish8_to_size(c.b, blue_size),
	};
	uint16_t dest = 0;
	dest |= squished.r << red_pos;
	dest |= squished.g << red_pos;
	dest |= squished.b << blue_pos;
	where[0] = dest;
	where[1] = (dest >> 8);
}

uint8_t VESADriver::squish8_to_size(int val, uint8_t destsize) {
	return (val * destsize) / 255;
}