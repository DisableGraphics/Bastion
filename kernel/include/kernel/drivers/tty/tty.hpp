#pragma once
#include <stddef.h>
#include <stdint.h>

constexpr size_t TTY_BLOCK_DISP = 4;
constexpr size_t TTY_BLOCK_SIZE = 16;

/**
	\brief TTY class.
	\details The TTY holds a buffer that contains all the characters written to it.
	The TTY manager is the one that does the work to display them.
 */
class TTY {
	public:
		TTY();
		~TTY();
		void init(size_t charwidth, size_t charheight);
		void putchar(char c);
		void write(const char* data, size_t size);
		void writestring(const char* data);
		uint8_t * get_buffer();
		bool* get_dirty_blocks();
		void clear();
	private:
		void handle_backspace();
		void putentryat(unsigned char c, size_t x, size_t y);

		size_t x = 0;
		size_t y = 0;
		size_t max_x = 0, max_y = 0, bufsize = 0;
		uint8_t* buffer;
		bool* dirty_blocks;
};