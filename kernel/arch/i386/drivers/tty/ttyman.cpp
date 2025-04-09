#include <stddef.h>
#include <kernel/drivers/tty/ttyman.hpp>
#include <string.h>

void TTYManager::init(hal::VideoDriver* screen) {
	for(size_t i = 0; i < N_TTYS; i++) {
		ttys[i].init(screen->get_font_width(), screen->get_font_height());
	}
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
	screen->flush();
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