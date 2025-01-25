#include <kernel/scheduler/mutex.hpp>
#include <kernel/scheduler/scheduler.hpp>

Mutex::Mutex() : locked(false) {

}

void Mutex::lock() {
	if(locked) {
		waiting_tasks.push_back(Scheduler::get().get_current_task());
		Scheduler::get().block(TaskState::WAITING);
	} else {
		locked = true;
	}
}

void Mutex::unlock() {
	if(waiting_tasks.size() > 0) {
		Task* task = waiting_tasks[0];
		waiting_tasks.erase(0);
		Scheduler::get().unblock(task);
	} else {
		locked = false;
	}
}