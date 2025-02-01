#include <kernel/hal/drvbase/timer.hpp>
#include <kernel/hal/managers/timermanager.hpp>

void hal::Timer::set_callback(void (*callback)()) {
	this->callback = callback;
}
