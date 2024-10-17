#include <stddef.h>

// TTY
class TTY {
    public:
        TTY();
        void putchar(char c);
        void write(const char* data, size_t size);
        void writestring(const char* data);
};