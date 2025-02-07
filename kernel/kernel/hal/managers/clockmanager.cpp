#include <kernel/hal/managers/clockmanager.hpp>
#include <kernel/kernel/log.hpp>

hal::ClockManager& hal::ClockManager::get() {
	static ClockManager instance;
	return instance;
}

void hal::ClockManager::set_clock(hal::Clock* clock) {
	if(!this->clock) {
		this->clock = clock;
	} else {
		log(ERROR, "Can't have more than one clock device.");
	}
}