#include <stddef.h>
#include <kernel/drivers/tty/ttyman.hpp>
#include <string.h>

void TTYManager::init() {

}

TTYManager &TTYManager::get() {
	static TTYManager instance;
	return instance;
}

void TTYManager::putchar(char c) {
	ttys[current_tty].putchar(c);
	display();
}

void TTYManager::write(const char *data, size_t size) {
	ttys[current_tty].write(data, size);
	display();
}

void TTYManager::writestring(const char * data) {
	ttys[current_tty].writestring(data);
	display();
}

void TTYManager::display() {
	memcpy(VGA_MEMORY, ttys[current_tty].get_buffer(), sizeof(uint16_t)*TTY::VGA_HEIGHT * TTY::VGA_WIDTH);
}

void TTYManager::set_current_tty(size_t tty) {
	if(tty >= N_TTYS) 
		tty = N_TTYS-1;
	current_tty = tty;
	display();
}

void TTYManager::clear() {
	ttys[current_tty].clear();
	display();
}