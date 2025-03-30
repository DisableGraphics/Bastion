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

void PIT::start(uint32_t interval_us) {
	int PIT_reload_value = (1193182 * interval_us) / tc::s;
	us_per_tick = interval_us;
	
	// Program the PIT channel
	IDT::get().disable_interrupts(); // Function to disable interrupts

	outb(0x43, 0x34); // Set command to PIT control register
	outb(0x40, PIT_reload_value & 0xFF); // Send low byte
	outb(0x40, (PIT_reload_value >> 8) & 0xFF); // Send high byte

	IDT::get().enable_interrupts();
	basic_setup(hal::TIMER);
}

size_t PIT::ellapsed() {
	return accum_us;
}

void PIT::sleep(uint32_t millis) {
	sleep_us = millis;
	while (sleep_us > 0) {
		halt();
	}
}