#include <kernel/tty/tty.hpp>
#include "../defs/vga/vga.hpp"
#include <kernel/drivers/cursor.hpp>
#include <string.h>

TTY::TTY() {
    terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void TTY::putchar(char c) {
    unsigned char uc = c;
	if(uc == '\b') {
		handle_backspace();
	} else if(uc == '\n'){
		terminal_column = 0;
		if(++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	} else {
		putentryat(uc, terminal_color, terminal_column, terminal_row);
		if (++terminal_column == VGA_WIDTH) {
			terminal_column = 0;
			if (++terminal_row == VGA_HEIGHT)
				terminal_row = 0;
		}
	}
	Cursor::get().move(terminal_column, terminal_row);
}

void TTY::write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
		putchar(data[i]);
}

void TTY::writestring(const char* data) {
    write(data, strlen(data));
}

void TTY::putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void TTY::handle_backspace() {
	if(terminal_column == 0) {
		terminal_column = VGA_WIDTH;
		terminal_row--;
	}
	terminal_column--;
	putentryat(' ', terminal_color, terminal_column, terminal_row);
}

uint16_t * TTY::get_buffer() {
	return terminal_buffer;
}