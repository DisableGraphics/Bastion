#include <kernel/drivers/cursor.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/tty/tty.hpp>

Cursor& Cursor::get() {
	static Cursor instance;
	return instance;
}

void Cursor::init() {
	enable(15,15);
}

void Cursor::enable(uint8_t cursor_start, uint8_t cursor_end) {
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void Cursor::disable() {
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void Cursor::move(int x, int y) {
	uint16_t pos = y * TTY::VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

uint16_t Cursor::get_position() {
	uint16_t pos = 0;
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos;
}