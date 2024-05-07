#include <kernel/tty.hpp>
#include <kernel/tty.h>

TTY::TTY() {
    terminal_initialize();
}

void TTY::putchar(char c) {
    terminal_putchar(c);
}

void TTY::write(const char* data, size_t size) {
    terminal_write(data, size);
}

void TTY::writestring(const char* data) {
    terminal_writestring(data);
}
