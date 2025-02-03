#include <kernel/drivers/interrupts.hpp>
#include <kernel/hal/drvbase/clock.hpp>

/**
	\brief Real time clock driver.
	Implemented as a singleton.
 */
class RTC final : public hal::Clock {
	public:
		RTC();
		/**
			\brief Initialises the Real Time Clock
			so it interrupts each second.
		 */
		void init() override;
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
			\brief Returns the value of register reg
			\param reg The register to read the value of
			\returns The value of the register reg
		 */
		uint8_t get_register(int reg);
		/**
			\brief Precompute days so we don't compute them constantly
		 */
		void compute_days();
		
		/// Total days until our current day since the UNIX epoch
		uint64_t total_days = 0;

		uint8_t second = 0;
		uint8_t minute = 0;
		uint8_t hour = 0;
		uint8_t day = 0;
		uint8_t month = 0;
		uint32_t year = 0;
		constexpr static uint32_t century = 2000;

		void update_timestamp() override;
};