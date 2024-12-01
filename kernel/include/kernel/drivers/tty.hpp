#include <stddef.h>
#include <stdint.h>

// TTY
class TTY {
    public:
		static TTY& get();
        void init();
        void putchar(char c);
        void write(const char* data, size_t size);
        void writestring(const char* data);

		const size_t VGA_WIDTH = 80;
		const size_t VGA_HEIGHT = 25;
	private:
		TTY(){};

		void handle_backspace();

		void putentryat(unsigned char c, uint8_t color, size_t x, size_t y);
		uint16_t* const VGA_MEMORY = (uint16_t*) 0xC03FF000;

		size_t terminal_row;
		size_t terminal_column;
		uint8_t terminal_color;
		uint16_t* terminal_buffer;
};