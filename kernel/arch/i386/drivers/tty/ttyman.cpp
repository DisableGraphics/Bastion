#include <stddef.h>
#include <kernel/drivers/tty/ttyman.hpp>
#include <string.h>

void TTYManager::init(hal::VideoDriver* screen) {
	this->screen = screen;
	font_width = screen->get_font_width();
	font_height = screen->get_font_height();
	width = screen->get_width();
	height = screen->get_height();

	char_width = screen->get_width() / font_width;
	char_height = screen->get_height() / font_height;
	charsize = char_height * char_width;
	for(size_t i = 0; i < N_TTYS; i++) {
		ttys[i].init(char_width, 
		char_height);
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
	uint8_t* buffer = ttys[current_tty].get_buffer();
	bool* dirty_blocks = ttys[current_tty].get_dirty_blocks();
	size_t sz = charsize >> TTY_BLOCK_DISP;
	for(size_t pos = 0; pos < sz; pos++) { 
		if(dirty_blocks[pos]) {
			const size_t lower_bound = pos << TTY_BLOCK_DISP;
			const size_t upper_bound = (pos + 1) << TTY_BLOCK_DISP; 	
			for(size_t i = 0; i < (1 << TTY_BLOCK_DISP); i++) {
				const size_t ps = (pos << TTY_BLOCK_DISP) + i;
				const size_t start_pos_x = (ps % char_width) * font_width;
				const size_t start_pos_y = (ps / char_width) * font_height;
				screen->draw_rectangle(start_pos_x, 
					start_pos_y, 
					start_pos_x + font_width,
					start_pos_y + font_height,
					{0,0,0,0});
			}
			
			for(size_t i = 0; i < (1 << TTY_BLOCK_DISP); i++) {
				const size_t ps = (pos << TTY_BLOCK_DISP) + i;
				const size_t start_pos_x = (ps % char_width) * font_width;
				const size_t start_pos_y = (ps / char_width) * font_height;
				screen->draw_char(buffer[ps], start_pos_x, start_pos_y);		
			}
			screen->flush();
			
			dirty_blocks[pos] = false;
		}
	}
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