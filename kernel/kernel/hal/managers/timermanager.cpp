#include <kernel/hal/managers/timermanager.hpp>

hal::TimerManager& hal::TimerManager::get() {
	static TimerManager instance;
	return instance;
}

void hal::TimerManager::register_timer(Timer* timer, uint32_t ms, void (*callback)()) {
	timers.push_back(timer);
	timers.back()->init();
	timers.back()->start(ms);
}

hal::Timer& hal::TimerManager::get_timer(size_t pos) {
	return *timers[pos];
}