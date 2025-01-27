#include <kernel/drivers/interrupts.hpp>

/**
	\brief Real time clock driver.
	Implemented as a singleton.
 */
class RTC {
	public:
		/**
			\brief Get singleton instance
		 */
		[[gnu::no_caller_saved_registers]]
		static RTC &get();
		/**
			\brief Initialises the Real Time Clock
			so it interrupts each second.
		 */
		void init();
	private:
		/**
			\brief Reads register C.
			\warning If the interrupt handler
			doesn't read the register C the RTC won't
			interrupt anymore.
		 */
		[[gnu::no_caller_saved_registers]]
		uint8_t read_register_c();
		/**
			\brief Interrupt handler for the Real Time Clock.
			\details It sets the time in TimeManager to the current time.
		 */
		[[gnu::interrupt]]
		static void interrupt_handler(interrupt_frame*);
		/**
			\brief Returns the value of register reg
			\param reg The register to read the value of
			\returns The value of the register reg
		 */
		uint8_t get_register(int reg);
		/**
			\brief Precompute days so we don't compute them constantly
		 */
		void compute_days();
		/**
			\brief Get timestamp of current second
		 */
		uint64_t get_timestamp();
		/// Total days until our current day since the UNIX epoch
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