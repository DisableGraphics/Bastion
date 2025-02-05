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
			void handle_interrupt() override {
				hal::IRQControllerManager::get().eoi(this->irqline);
				sleep_ms -= ms_per_tick;
				if(callback) callback();
				if(is_scheduler_timer) scheduler_callback();
			}
			void set_is_scheduler_timer(bool is_scheduler_timer) {
				this->is_scheduler_timer = is_scheduler_timer;
			}
			virtual void scheduler_callback() {
				Scheduler &sch = Scheduler::get();
				sch.handle_sleeping_tasks();
				sch.preemptive_scheduling();
			}
		protected:
			void (*callback)() = nullptr;
			bool is_scheduler_timer = false;
			size_t ms_per_tick = 0;
			size_t accum_ms = 0;
			volatile int sleep_ms = 0;
	};
}