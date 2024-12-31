#include <kernel/drivers/rtc.hpp>
#include <kernel/drivers/nmi.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/pic.hpp>

#include <stdio.h>

RTC& RTC::get() {
	static RTC instance;
	return instance;
}

void RTC::init() {
	IDT::get().set_handler(PIC::get().get_offset() + 8, RTC::interrupt_handler);
	IDT::disable_interrupts();
	NMI::disable();
	
	outb(0x70, 0x8B);
	char prev = inb(0x71);

	outb(0x70, 0x8B);

	outb(0x71, prev | 0x40);
	
	read_register_c();

	PIC::get().IRQ_clear_mask(8);

	NMI::enable();
	IDT::enable_interrupts();
}

void RTC::interrupt_handler(interrupt_frame*) {
	IDT::disable_interrupts();
	RTC::get().read_register_c();

	PIC::get().send_EOI(8);
	IDT::enable_interrupts();
}

uint8_t RTC::read_register_c() {
	outb(0x70, 0x0C);
	return inb(0x71);
}