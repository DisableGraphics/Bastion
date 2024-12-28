#include <kernel/drivers/nmi.hpp>
#include <kernel/assembly/inlineasm.h>

NMI nmi;

void NMI::disable() {
	outb(0x70, inb(0x70) | 0x80);
	inb(0x71);
}

void NMI::enable() {
	outb(0x70, inb(0x70) & 0x7F);
	inb(0x71);
}