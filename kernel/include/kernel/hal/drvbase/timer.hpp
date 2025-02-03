#pragma once
#include <kernel/drivers/interrupts.hpp>
#include <kernel/hal/drvbase/driver.hpp>

namespace hal {
	/**
		\brief HAL base class timer.
	 */
	class Timer : public Driver {
		public:
			~Timer() = default;
			virtual void init() override = 0;
			/**
				\brief Starts the timer with a period of interval_ms.
				\param interval_ms Number of milliseconds between ticks.
			 */
			virtual void start(uint32_t interval_ms) = 0;
			/**
				\brief Stop the timer.
			 */
			virtual void stop() = 0;
			/**
				\brief Get ellapsed time in milliseconds
			 */
			virtual size_t ellapsed() = 0;
		protected:

	};
}