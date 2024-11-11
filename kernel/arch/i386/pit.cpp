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
	
	uint32_t eax, ebx, edx;

    // Do some checking and determine the reload value
    eax = 0x10000; // Slowest possible frequency (65536)

    if (freq <= 18) {
    } else if (freq >= 1193181) {
        eax = 1; // Use the fastest possible frequency
    } else {
        // Calculate reload value
        eax = 3579545;
        edx = 0;
        eax /= freq; // eax = 3579545 / frequency, edx = remainder
        if ((3579545 % freq) > (3579545 / 2)) {
            eax++; // Round up if remainder > half
        }
    }

    // Store reload value and calculate actual frequency
    PIT_reload_value = eax;
    ebx = eax;

    eax = 3579545;
    edx = 0;
    eax /= ebx; // eax = 3579545 / reload_value
    if ((3579545 % ebx) > (3579545 / 2)) {
        eax++; // Round up if remainder > half
    }
    IRQ0_frequency = eax;

    // Calculate the time in milliseconds between IRQs in 32.32 fixed point
    ebx = PIT_reload_value;
    eax = 0xDBB3A062; // eax = 3000 * (2^42) / 3579545

    uint64_t product = (uint64_t)eax * ebx;
    eax = (product >> 10) & 0xFFFFFFFF; // Get the lower 32 bits after shifting
    edx = product >> 42;                // Get the higher 32 bits after shifting

    IRQ0_ms = edx;        // Whole ms between IRQs
    IRQ0_fractions = eax; // Fractions of 1 ms between IRQs

    // Program the PIT channel (abstracted in this C version)
    idt.disable_interrupts(); // Function to disable interrupts

    outb(0x43, 0x34);        // Set command to PIT control register
    outb(0x40, PIT_reload_value & 0xFF);      // Send low byte
    outb(0x40, (PIT_reload_value >> 8) & 0xFF); // Send high byte

    idt.enable_interrupts();
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
	pit.countdown -= pit.IRQ0_ms;
	outb(PIC1, PIC_EOI);
}

void PIT::sleep(uint32_t millis) {
    countdown = millis;
    while (countdown > 0) {
        halt();
    }
}
