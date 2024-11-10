#include <kernel/pit.hpp>
#include <kernel/inlineasm.h>
#include <kernel/interrupts.hpp>
#include "defs/pic/pic.hpp"
#include <kernel/pic.hpp>
#include <stdio.h>

PIT pit;

void PIT::init(int freq) {
	idt.set_handler(0x20, pit_handler);
	pic.IRQ_clear_mask(0);
	
	int final_freq;
	if(freq < 18) {
		final_freq = 65536;
	} else if(freq > 1193181) {
		final_freq = 1;
	} else {
		final_freq = 3579545;
	}
}

uint16_t PIT::read_count() {
	uint16_t count;
	// Disable interrupts
	idt.disable_interrupts();
	
	// al = channel in bits 6 and 7, remaining bits clear
	outb(PIT_MODE_COMMAND,0b0000000);
	
	count = inb(0x40);		// Low byte
	count |= inb(0x40) << 8;	// High byte
	idt.enable_interrupts();
	return count;
}

void PIT::set_count(uint16_t count) {
	// Disable interrupts
	idt.disable_interrupts();
	
	// Set low byte
	outb(PIT_CHAN0, count & 0xFF);		// Low byte
	outb(PIT_CHAN0, (count & 0xFF00) >> 8);	// High byte
	idt.enable_interrupts();
}

void PIT::pit_handler(interrupt_frame *) {
	pit.system_timer_fractions += pit.IRQ0_fractions;
	pit.system_timer_ms += pit.IRQ0_ms;
	outb(PIC1, PIC_EOI);
	printf(".");
}