#pragma once
#include <stddef.h>
#include <stdint.h>

/**
	\brief TTY class.
	\details The TTY holds a buffer that contains all the characters written to it.
	The TTY manager is the one that does the work to display them.
 */
class TTY {
	public:
		TTY();
		void init();
		void putchar(char c);
		void write(const char* data, size_t size);
		void writestring(const char* data);
		uint16_t * get_buffer();
		void clear();

		const static size_t VGA_WIDTH = 80;
		const static size_t VGA_HEIGHT = 25;
	private:
		void handle_backspace();
		void putentryat(unsigned char c, uint8_t color, size_t x, size_t y);

		size_t terminal_row;
		size_t terminal_column;
		uint8_t terminal_color;
		uint16_t terminal_buffer[VGA_HEIGHT * VGA_WIDTH];
};