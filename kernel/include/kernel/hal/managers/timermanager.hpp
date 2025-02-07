#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/uptr.hpp>
#include <kernel/hal/drvbase/timer.hpp>

namespace hal {
	class TimerManager {
		public:
			static TimerManager& get();
			void register_timer(Timer* timer, uint32_t ms, void (*callback)() = nullptr);
			Timer& get_timer(size_t pos);
			void exec_at(uint32_t ms, void (*fn)(volatile void*), volatile void* args);
		private:
			TimerManager() = default;

			TimerManager(const TimerManager&) = delete;
		    TimerManager& operator=(const TimerManager&) = delete;

			Vector<Timer*> timers;
	};
}