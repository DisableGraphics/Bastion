#pragma once
#include <kernel/hal/drvbase/clock.hpp>

namespace hal {
	class ClockManager {
		public:
			static ClockManager& get();
			void set_clock(Clock* clock);
		private:
			Clock* clock = nullptr;

			ClockManager(){};
			ClockManager(const ClockManager&) = delete;
			ClockManager(ClockManager&&) = delete;
			void operator=(const ClockManager&) = delete;
			void operator=(ClockManager&&) = delete;
	};
}