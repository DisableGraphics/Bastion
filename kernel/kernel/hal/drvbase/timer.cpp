#include "kernel/scheduler/scheduler.hpp"
#include <kernel/hal/drvbase/timer.hpp>
#include <kernel/hal/managers/timermanager.hpp>

void hal::Timer::handle_interrupt() {
	hal::IRQControllerManager::get().eoi(this->irqline);
	// Handle sleep + the future execution
	sleep_us -= us_per_tick;
	exec_future_us -= us_per_tick;
	if(exec_future_fn && exec_future_us <= 0) {
		exec_future_fn(exec_future_args);
		exec_future_fn = nullptr;
		exec_future_args = nullptr;
	}
	// Call callback for the timer
	if(callback) callback();
	// If the timer is a scheduler timer, call the scheduler's callback
	if(is_scheduler_timer) scheduler_callback();
}

void hal::Timer::set_is_scheduler_timer(bool is_scheduler_timer) {
	this->is_scheduler_timer = is_scheduler_timer;
	Scheduler::get().set_clock_tick(us_per_tick);
}

void hal::Timer::scheduler_callback() {
	Scheduler &sch = Scheduler::get();
	sch.handle_sleeping_tasks();
	sch.preemptive_scheduling();
}

void hal::Timer::exec_at(tc::timertime us, void (*fn)(volatile void*), volatile void* args) {
	exec_future_fn = fn;
	exec_future_us = us;
	exec_future_args = args;
}