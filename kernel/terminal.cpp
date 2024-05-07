#include "terminal.hpp"
#include "vga.hpp"
#include "klibc/string.h"

Terminal::Terminal() {
    terminal_row = 0;
	terminal_column = 0;
	terminal_color = VGA::entry_color(VGA::VGA_COLOR_LIGHT_GREY, VGA::VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = VGA::entry(' ', terminal_color);
		}
	}

}

void Terminal::set_color(uint8_t color)
{
    terminal_color = color;
}

void Terminal::putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = VGA::entry(c, color);
}

void Terminal::putchar(char c) {
    putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}

void Terminal::write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		putchar(data[i]);
}

void Terminal::writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}