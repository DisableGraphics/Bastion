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
			/**
				\brief Initialise the timer.
				\details Sets up the hardware timer.
			 */
			virtual void init() = 0;
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
				\brief Sets the callback function to run when the timer ticks.
			 */
			void set_callback(void (*callback)());
		protected:
			/// Callback function that runs when the timer interrupts
			void (*callback)() = nullptr;
	};
}