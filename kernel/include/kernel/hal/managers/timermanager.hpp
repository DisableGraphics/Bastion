#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/uptr.hpp>
#include <kernel/hal/drvbase/timer.hpp>
#include <kernel/kernel/timeconst.hpp>

namespace hal {
	class TimerManager {
		public:
			static TimerManager& get();
			void register_timer(Timer* timer, tc::timertime us, void (*callback)() = nullptr);
			Timer& get_timer(size_t pos);
			void exec_at(tc::timertime us, void (*fn)(volatile void*), volatile void* args);
		private:
			TimerManager() = default;

			TimerManager(const TimerManager&) = delete;
		    TimerManager& operator=(const TimerManager&) = delete;

			Vector<Timer*> timers;
	};
}