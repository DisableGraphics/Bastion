#include <stdint.h>
#include <kernel/drivers/pit.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/scheduler/scheduler.hpp>
#include "../arch/i386/defs/pic/pic.hpp"
#include <kernel/drivers/pic.hpp>

void PIT::init(int freq) {
	IDT::get().set_handler(PIC::get().get_offset() + 0, pit_handler);
	PIC::get().IRQ_clear_mask(0);
	
	uint32_t final_freq, ebx, edx;

	for(uint32_t i = 0; i < K_N_COUNTDOWNS; i++) {
		callbacks[i] = nullptr;
		callback_args[i] = nullptr;
	}

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
	PIT_reload_value = final_freq;
	ebx = final_freq;

	final_freq = 3579545;
	edx = 0;
	final_freq /= ebx; // eax = 3579545 / reload_value
	if ((3579545 % ebx) > (3579545 / 2)) {
		final_freq++; // Round up if remainder > half
	}
	IRQ0_frequency = final_freq;

	// Calculate the time in milliseconds between IRQs in 32.32 fixed point
	ebx = PIT_reload_value;
	final_freq = 0xDBB3A062; // 3000 * (2^42) / 3579545

	uint64_t product = (uint64_t)final_freq * ebx;
	final_freq = (product >> 10) & 0xFFFFFFFF; // Get the lower 32 bits after shifting
	edx = product >> 42; // Get the higher 32 bits after shifting

	IRQ0_ms = edx; // Whole ms between IRQs
	IRQ0_fractions = final_freq; // Fractions of 1 ms between IRQs

	// Program the PIT channel (abstracted in this C version)
	IDT::get().disable_interrupts(); // Function to disable interrupts

	outb(0x43, 0x34); // Set command to PIT control register
	outb(0x40, PIT_reload_value & 0xFF); // Send low byte
	outb(0x40, (PIT_reload_value >> 8) & 0xFF); // Send high byte

	IDT::get().enable_interrupts();
	Scheduler::get().set_clock_tick(IRQ0_ms);
}

PIT &PIT::get() {
	static PIT instance;
	return instance;
}

uint16_t PIT::read_count() {
	uint16_t count;
	// Disable interrupts
	IDT::get().disable_interrupts();
	
	// al = channel in bits 6 and 7, remaining bits clear
	outb(PIT_MODE_COMMAND,0b0000000);
	
	count = inb(0x40);		// Low byte
	count |= inb(0x40) << 8;	// High byte
	IDT::get().enable_interrupts();
	return count;
}

void PIT::set_count(uint16_t count) {
	// Disable interrupts
	IDT::get().disable_interrupts();
	
	// Set low byte
	outb(PIT_CHAN0, count & 0xFF);		// Low byte
	outb(PIT_CHAN0, (count & 0xFF00) >> 8);	// High byte
	IDT::get().enable_interrupts();
}

void PIT::pit_handler(interrupt_frame *) {
	PIT& pit = PIT::get();
	uint32_t ms = pit.IRQ0_ms;
	pit.system_timer_fractions += pit.IRQ0_fractions;
	pit.system_timer_ms += pit.IRQ0_ms;
	PIC::get().send_EOI(0);
	for(uint32_t i = 0; i < K_N_COUNTDOWNS; i++) {
		if(pit.allocated & (1 << i)) {
			pit.kernel_countdowns[i] -= pit.IRQ0_ms;
			if(pit.kernel_countdowns[i] < 0 && pit.callbacks[i] != nullptr) {
				pit.callbacks[i](pit.callback_args[i]);
			}
		}
	}
	Scheduler &sch = Scheduler::get();
	sch.handle_sleeping_tasks();
	sch.preemptive_scheduling();
}

void PIT::sleep(uint32_t millis) {
	uint32_t handle = alloc_timer();
	if(handle == UINT32_MAX) return;
	kernel_countdowns[handle] = millis;
	while (kernel_countdowns[handle] > 0) {
		halt();
	}
	dealloc_timer(handle);
}

uint32_t PIT::timer_callback(uint32_t handle, uint32_t millis, void (*fn)(void*), void *arg) {
	if(handle == UINT32_MAX) {
		handle = alloc_timer();
		if(handle == UINT32_MAX)
			return handle;
	}
	
	kernel_countdowns[handle] = millis;
	callbacks[handle] = fn;
	callback_args[handle] = arg;

	return handle;
}

void PIT::clear_callback(uint32_t handle) {
	dealloc_timer(handle);
	callbacks[handle] = nullptr;
	callback_args[handle] = nullptr;
}

uint32_t PIT::alloc_timer() {
	if(this->allocated == UINT32_MAX)
		return UINT32_MAX;
	uint32_t i;
	for(i = 0; i < K_N_COUNTDOWNS; i++) {
		if(!(allocated & (1 << i))) {
			kernel_countdowns[i] = 0;
			this->allocated |= (1 << i);
			break;
		}
	}
	return i;
}

void PIT::dealloc_timer(uint32_t handle) {
	if(handle == UINT32_MAX) return;
	allocated &= (UINT32_MAX & ~(1 << handle));
}

uint32_t PIT::millis_accum() {
	return system_timer_ms;
}