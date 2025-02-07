#include "kernel/scheduler/scheduler.hpp"
#include <kernel/hal/drvbase/timer.hpp>
#include <kernel/hal/managers/timermanager.hpp>

void hal::Timer::handle_interrupt() {
	hal::IRQControllerManager::get().eoi(this->irqline);
	sleep_ms -= ms_per_tick;
	if(callback) callback();
	if(is_scheduler_timer) scheduler_callback();
}

void hal::Timer::set_is_scheduler_timer(bool is_scheduler_timer) {
	this->is_scheduler_timer = is_scheduler_timer;
	Scheduler::get().set_clock_tick(ms_per_tick);
}

void hal::Timer::scheduler_callback() {
	Scheduler &sch = Scheduler::get();
	sch.handle_sleeping_tasks();
	sch.preemptive_scheduling();
}