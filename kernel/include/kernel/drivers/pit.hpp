#pragma once
#include <kernel/drivers/interrupts.hpp>
#include <kernel/hal/drvbase/timer.hpp>
#ifdef __i386
#include "../arch/i386/defs/pit/pit.hpp"
#endif

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
		void start(tc::timertime us) override;
		void stop() override;
		tc::timertime ellapsed() override;
		void sleep(tc::timertime us) override;
	private:
		/**
			\brief Initialise timer
			\param freq Frequency of the timer in Hertz (Hz)
		 */
		void init(int freq);
};