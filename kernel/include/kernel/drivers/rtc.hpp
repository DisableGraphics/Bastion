#include <kernel/drivers/interrupts.hpp>

class RTC {
	public:
		[[gnu::no_caller_saved_registers]]
		static RTC &get();
		void init();
		
	private:
		[[gnu::no_caller_saved_registers]]
		uint8_t read_register_c();
		[[gnu::interrupt]]
		static void interrupt_handler(interrupt_frame*);
		uint8_t get_register(int reg);

		void compute_days();

		uint64_t get_timestamp();

		uint64_t total_days = 0;

		uint8_t second = 0;
		uint8_t minute = 0;
		uint8_t hour = 0;
		uint8_t day = 0;
		uint8_t month = 0;
		uint32_t year = 0;
		constexpr static uint32_t century = 2000;

		RTC(){};
};