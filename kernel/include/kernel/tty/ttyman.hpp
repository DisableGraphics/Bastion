#include <stdint.h>
#include <stddef.h>
#include <kernel/tty/tty.hpp>

class TTYManager {
	public:
		static TTYManager& get();
		void init();

		void putchar(char c);
        void write(const char* data, size_t size);
        void writestring(const char* data);
		void set_current_tty(size_t tty);
	private:
		void display();
		TTYManager(){}
		size_t current_tty = 0;
		const static size_t N_TTYS = 5;
		TTY ttys[N_TTYS];
		uint16_t* const VGA_MEMORY = (uint16_t*) 0xC03FF000;
};