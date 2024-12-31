#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/serial.hpp>

#define PORT 0x3f8 // COM1

Serial &Serial::get() {
	static Serial instance;
	return instance;
}

void Serial::init() {
	outb(PORT + 1, 0x00); // Disable all interrupts
	outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	outb(PORT + 1, 0x00); // (hi byte)
	outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
	outb(PORT + 4, 0x1E); // Set in loopback mode, test the serial chip
	outb(PORT + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	if(inb(PORT + 0) != 0xAE) {
		faulty = true;
		return;
	}

	// If serial is not faulty set it in normal operation mode
	// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
	outb(PORT + 4, 0x0F);
}

bool Serial::is_faulty() {
	return faulty;
}

bool Serial::received() {
	return inb(PORT + 5) & 1; 
}

char Serial::read() {
	while(!received())
		;
   	return inb(PORT);
}

bool Serial::is_transmit_empty() {
	return inb(PORT + 5) & 0x20;
}

void Serial::write(char a) {
   while(!is_transmit_empty());
   outb(PORT,a);
}

void Serial::print(const char * str) {
	for(; *str != 0; str++) {
		write(*str);
	}
}
