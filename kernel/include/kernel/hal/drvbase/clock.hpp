#pragma once
#include <time.h>
#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/time/time.hpp>

namespace hal {
	class Clock : public Driver {
		public:
			/**
				\brief Initialise device. Device must interrupt each second.
			 */
			virtual void init() override = 0;
			virtual time_t get_timestamp() { return timestamp; };
			virtual void handle_interrupt() override {
				update_timestamp();
				TimeManager::get().set_time(get_timestamp());
				TimeManager::get().next_second();
			}
		protected:
			time_t timestamp = 0;
			/**
				\brief Updates the variable timestamp to the current one.
				\details timestamp must be in UNIX time format (seconds since 1/1/1970 00:00:00)
				\note the class TimeManager has a few functions that can help with these calculations
			 */
			virtual void update_timestamp() = 0;
	};
}