#pragma once
#include "kernel/interrupts.hpp"
#include <stdint.h>
#ifdef __i386
#include "../arch/i386/defs/pit/pit.hpp"
#endif

class PIT {
	public:
		static PIT &get();

		void init(int freq);
		uint16_t read_count();
		void set_count(uint16_t count);
		void sleep(uint32_t millis);
		[[gnu::interrupt]]
		static void pit_handler(interrupt_frame*);
	private:
		uint32_t system_timer_fractions = 0, system_timer_ms = 0;
		uint32_t IRQ0_fractions = 0, IRQ0_ms = 0;
		uint32_t IRQ0_frequency = 0;
		uint16_t PIT_reload_value = 0;

		volatile int64_t countdown;

		PIT(){}
};