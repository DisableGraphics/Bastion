#include <time.h>
#include <kernel/time/time.hpp>

time_t seconds_since_boot() {
	return TimeManager::get().get_seconds_since_boot();
}