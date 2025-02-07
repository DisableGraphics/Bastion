#include <kernel/hal/managers/timermanager.hpp>

hal::TimerManager& hal::TimerManager::get() {
	static TimerManager instance;
	return instance;
}

void hal::TimerManager::register_timer(Timer* timer, uint32_t ms, void (*callback)()) {
	timers.push_back(timer);
	timers.back()->start(ms);
}

hal::Timer& hal::TimerManager::get_timer(size_t pos) {
	return *timers[pos];
}

void hal::TimerManager::exec_at(uint32_t ms, void (*fn)(volatile void*), volatile void* args) {
	static int seltimer = 0;
	if(timers.size() > 0) {
		timers[seltimer]->exec_at(ms, fn, args);
	}

	seltimer++;
	if(timers.size() == 0)
		seltimer = 0;
	else 
		seltimer = seltimer % timers.size();
}