#pragma once
#include <kernel/drivers/interrupts.hpp>
#include <stdint.h>
#include <string.h>
#include <kernel/hal/drvbase/timer.hpp>
#ifdef __i386
#include "../arch/i386/defs/pit/pit.hpp"
#endif

// Number of max kernel countdowns available
constexpr uint32_t K_N_COUNTDOWNS = 32;
/**
	\brief Programmable Interrupt Timer driver
	Implemented as a singleton

	Not a high resolution timer.
*/
class PIT final : public hal::Timer {
	public:
		PIT();
		virtual ~PIT();
		void init() override;
		void start(uint32_t ms) override;
		void stop() override;
		size_t ellapsed() override;
		/**
			\brief Sleep for millis milliseconds
			\param millis The number of milliseconds to sleep
		 */
		void sleep(uint32_t millis) override;
	private:
		/**
			\brief Initialise timer
			\param freq Frequency of the timer in Hertz (Hz)
		 */
		void init(int freq);
};