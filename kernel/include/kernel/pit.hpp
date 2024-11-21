#pragma once
#include "kernel/interrupts.hpp"
#include <stddef.h>
#include <stdint.h>
#ifdef __i386
#include "../arch/i386/defs/pit/pit.hpp"
#endif

// Number of max kernel countdowns available
constexpr uint32_t K_N_COUNTDOWNS = 32;

class PIT {
	public:
		static PIT &get();

		void init(int freq);
		uint16_t read_count();
		void set_count(uint16_t count);
		void sleep(uint32_t millis);

		uint32_t timer_callback(uint32_t handle, uint32_t millis, void (*fn)(void*), void *arg);
		void clear_callback(uint32_t handle);

		uint32_t alloc_timer();
		void dealloc_timer(uint32_t handle);

		[[gnu::interrupt]]
		static void pit_handler(interrupt_frame*);
	private:
		uint32_t system_timer_fractions = 0, system_timer_ms = 0;
		uint32_t IRQ0_fractions = 0, IRQ0_ms = 0;
		uint32_t IRQ0_frequency = 0;
		uint16_t PIT_reload_value = 0;

		volatile int kernel_countdowns[K_N_COUNTDOWNS];
		void (*callbacks[K_N_COUNTDOWNS])(void*);
		void *callback_args[K_N_COUNTDOWNS];
		uint32_t allocated = 0;

		PIT(){}
};