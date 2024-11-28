#include <stdint.h>

class Cursor {
	public:
		static Cursor &get();
		void init();
		void enable(uint8_t cursor_start, uint8_t cursor_end);
		void disable();
		void move(int x, int y);
		uint16_t get_position();
	private:
		Cursor(){};
};