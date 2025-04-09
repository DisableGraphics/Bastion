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
		void init(size_t charwidth, size_t charheight);
		void putchar(char c);
		void write(const char* data, size_t size);
		void writestring(const char* data);
		uint16_t * get_buffer();
		void clear();
	private:
		void handle_backspace();
		void putentryat(unsigned char c, size_t x, size_t y);

		size_t x = 0;
		size_t y = 0;
		size_t max_x = 0, max_y = 0;
		uint8_t* buffer;
};