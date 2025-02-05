#include <stdint.h>
#include <kernel/drivers/pit.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/scheduler/scheduler.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include "../arch/i386/defs/pic/pic.hpp"
#include <kernel/drivers/pic.hpp>

PIT::PIT() {

}

PIT::~PIT() {
	stop();
}

void PIT::init() {

}

void PIT::stop() {
	// Does a trick in which it reprograms the PIT to have 0 frequency in the one-shot mode
	outb(0x43, 0x30);
	outb(0x40, 0x00);
	outb(0x40, 0x00);
}

void PIT::start(uint32_t interval_ms) {
	int freq = 1000 / interval_ms;
	uint32_t final_freq, ebx, edx;

	// Do some checking and determine the reload value
	final_freq = 0x10000; // Slowest possible frequency (65536)

	if (freq <= 18) {
	} else if (freq >= 1193181) {
		final_freq = 1; // Use the fastest possible frequency
	} else {
		// Calculate reload value
		final_freq = 3579545;
		edx = 0;
		final_freq /= freq; // eax = 3579545 / frequency, edx = remainder
		edx = 3579545 % freq;
		if ((3579545 % freq) > (3579545 / 2)) {
			final_freq++; // Round up if remainder > half
		}
	}

	// Store reload value and calculate actual frequency
	auto PIT_reload_value = final_freq;
	ebx = final_freq;

	final_freq = 3579545;
	edx = 0;
	final_freq /= ebx; // eax = 3579545 / reload_value
	if ((3579545 % ebx) > (3579545 / 2)) {
		final_freq++; // Round up if remainder > half
	}
	
	// Calculate the time in milliseconds between IRQs in 32.32 fixed point
	ebx = PIT_reload_value;
	final_freq = 0xDBB3A062; // 3000 * (2^42) / 3579545

	uint64_t product = (uint64_t)final_freq * ebx;
	final_freq = (product >> 10) & 0xFFFFFFFF; // Get the lower 32 bits after shifting
	edx = product >> 42; // Get the higher 32 bits after shifting

	ms_per_tick = edx; // Whole ms between IRQs
	
	// Program the PIT channel (abstracted in this C version)
	IDT::get().disable_interrupts(); // Function to disable interrupts

	outb(0x43, 0x34); // Set command to PIT control register
	outb(0x40, PIT_reload_value & 0xFF); // Send low byte
	outb(0x40, (PIT_reload_value >> 8) & 0xFF); // Send high byte

	IDT::get().enable_interrupts();
	Scheduler::get().set_clock_tick(ms_per_tick);
	basic_setup(hal::PIT);
}

size_t PIT::ellapsed() {
	return accum_ms;
}

void PIT::sleep(uint32_t millis) {
	sleep_ms = millis;
	while (sleep_ms > 0) {
		halt();
	}
}