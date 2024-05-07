#include <stddef.h>
#include <stdint.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

class Terminal {
    public:
        Terminal();
        void writestring(const char * str);
        void set_color(uint8_t color);
    private:
        void putentryat(char c, uint8_t color, size_t x, size_t y);
        void putchar(char c);
        void write(const char* data, size_t size);
        size_t terminal_row;
        size_t terminal_column;
        uint8_t terminal_color;
        uint16_t* terminal_buffer;

};