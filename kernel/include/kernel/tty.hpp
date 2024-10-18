#include <stddef.h>

// TTY
class TTY {
    public:
        void init();
        void putchar(char c);
        void write(const char* data, size_t size);
        void writestring(const char* data);
};

extern TTY tty;