#include <kernel/drivers/interrupts.hpp>

class RTC {
	public:
		static RTC &get();
		void init();
	private:
		uint8_t read_register_c();
		[[gnu::interrupt]]
		static void interrupt_handler(interrupt_frame*);
		RTC(){};
};