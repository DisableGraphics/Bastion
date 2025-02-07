#pragma once
#include "kernel/hal/managers/irqcmanager.hpp"
#include <stdint.h>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/scheduler/scheduler.hpp>

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
			virtual void sleep(uint32_t ms) = 0;
			virtual void set_callback(void (*callback)()) {this->callback = callback;}
			void handle_interrupt() override;
			void set_is_scheduler_timer(bool is_scheduler_timer);
			void scheduler_callback();

			void exec_at(uint32_t ms, void(*fn)(volatile void*), volatile void* args);
		protected:
			void (*callback)() = nullptr;
			void (*exec_future_fn)(volatile void*) = nullptr;
			volatile void* exec_future_args = nullptr;
			bool is_scheduler_timer = false;
			size_t ms_per_tick = 0;
			size_t accum_ms = 0;
			volatile int sleep_ms = 0, exec_future_ms = 0;
	};
}