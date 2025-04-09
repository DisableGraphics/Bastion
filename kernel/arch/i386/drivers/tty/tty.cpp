#include <kernel/drivers/tty/tty.hpp>
#include "../../defs/vga/vga.hpp"
#include <string.h>

TTY::TTY() {

}

void TTY::putchar(char c) {
	unsigned char uc = c;
	if(uc == '\b') {
		handle_backspace();
	} else if(uc == '\n'){
		x = 0;
		if(++y == VGA_HEIGHT)
			y = 0;
	} else {
		putentryat(uc, x, y);
		if (++x == VGA_WIDTH) {
			x = 0;
			if (++y == VGA_HEIGHT)
				y = 0;
		}
	}
}

void TTY::write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		putchar(data[i]);
}

void TTY::writestring(const char* data) {
	write(data, strlen(data));
}

void TTY::putentryat(unsigned char c, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void TTY::handle_backspace() {
	if(x == 0) {
		x = VGA_WIDTH;
		y--;
	}
	x--;
	putentryat(' ', terminal_color, x, y);
}

uint16_t * TTY::get_buffer() {
	return terminal_buffer;
}

void TTY::clear() {
	y = 0;
	x = 0;
	for(size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++)
		putchar(' ');
}