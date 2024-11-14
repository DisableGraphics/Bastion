#pragma once

struct interrupt_frame;

class RTC {
	public:
		[[gnu::no_caller_saved_registers]]
		static RTC &get();
		void init();
		[[gnu::interrupt]]
		static void rtc_handler(interrupt_frame*);
	private:
		void interrupt_rate(int rate);
		void init_interrupts();
		[[gnu::no_caller_saved_registers]]
		void poll_register_c();
		RTC(){}
};