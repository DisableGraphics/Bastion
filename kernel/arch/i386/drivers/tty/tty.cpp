#include <kernel/drivers/tty/tty.hpp>
#include <string.h>
#include <kernel/kernel/log.hpp>

TTY::TTY() {

}

TTY::~TTY() {
	delete[] buffer;
	delete[] dirty_blocks;
}

void TTY::init(size_t charwidth, size_t charheight) {
	max_x = charwidth;
	max_y = charheight;
	bufsize = max_x * max_y;
	buffer = new uint8_t[bufsize];
	sse2_memset(buffer, ' ', bufsize);
	dirty_blocks = new bool[bufsize >> TTY_BLOCK_DISP];
}

void TTY::putchar(char c) {
	unsigned char uc = c;
	if(uc == '\b') {
		handle_backspace();
	} else if(uc == '\n'){
		x = 0;
		if(++y == max_y)
			y = 0;
	} else {
		putentryat(uc, x, y);
		if (++x == max_x) {
			x = 0;
			if (++y == max_y)
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
	const size_t index = (y * max_x) + x;
	buffer[index] = c;
	dirty_blocks[index >> TTY_BLOCK_DISP] = true;
}

void TTY::handle_backspace() {
	if(x == 0) {
		x = max_x;
		if(y == 0) y = max_y;
		y--;
	}
	x--;
	putentryat(' ', x, y);
}

uint8_t * TTY::get_buffer() {
	return buffer;
}

void TTY::clear() {
	y = 0;
	x = 0;
	for(size_t i = 0; i < bufsize; i++)
		putchar(' ');
}

bool* TTY::get_dirty_blocks() {
	return dirty_blocks;
}