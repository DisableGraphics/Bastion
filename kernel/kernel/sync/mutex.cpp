#include <kernel/sync/mutex.hpp>
#include <kernel/scheduler/scheduler.hpp>
#include <kernel/drivers/interrupts.hpp>

Mutex::Mutex() : locked(false) {

}

void Mutex::lock() {
	IDT::disable_interrupts();
	if(locked) {
		waiting_tasks.push_back(Scheduler::get().get_current_task());
		Scheduler::get().block(TaskState::WAITING);
	} else {
		locked = true;
	}
}

void Mutex::unlock() {
	IDT::disable_interrupts();
	if(waiting_tasks.size() > 0) {
		Task* task = waiting_tasks[0];
		waiting_tasks.erase(0);
		Scheduler::get().unblock(task);
	} else {
		locked = false;
	}
}