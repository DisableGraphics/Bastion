#include <kernel/rtc.hpp>
#include <kernel/nmi.hpp>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <kernel/pic.hpp>
#include <stdio.h>

void RTC::init() {
	idt.disable_interrupts();

	outb(0x70, 0x8A);	// select Status Register A, and disable NMI (by setting the 0x80 bit)
	outb(0x71, 0x20);	// write to CMOS/RTC RAM

	pic.IRQ_clear_mask(8);
	poll_register_c();
	init_interrupts();
	
	idt.enable_interrupts();
	nmi.enable();	
}

void RTC::init_interrupts() {
	outb(0x70, 0x8B);		// select register B, and disable NMI
	char prev = inb(0x71);	// read the current value of register B
	outb(0x70, 0x8B);		// set the index again (a read will reset the index to register D)
	outb(0x71, prev | 0x40);	// write the previous value ORed with 0x40. This turns on bit 6 of register B

	idt.set_handler(0x28, rtc_handler);
}

void RTC::poll_register_c() {
	outb(0x70, 0x0C);	// select register C
	inb(0x71);
}

void RTC::interrupt_rate(int rate) {
	rate &= 0x0F;					// rate must be above 2 and not over 15
	idt.disable_interrupts();
	outb(0x70, 0x8A);	// set index to register A, disable NMI
	char prev = inb(0x71);	// get initial value of register A
	outb(0x70, 0x8A);	// reset index to A
	outb(0x71, (prev & 0xF0) | rate); //write only our rate to A. Note, rate is the bottom 4 bits.
	idt.enable_interrupts();

}

void RTC::rtc_handler(interrupt_frame*) {
	printf(".");
	idt.disable_interrupts();
	RTC::get().poll_register_c();
	idt.enable_interrupts();
}

RTC& RTC::get() {
	static RTC instance;
	return instance;
}