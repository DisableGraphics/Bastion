#include <kernel/scheduler/semaphore.hpp>
#include <kernel/scheduler/scheduler.hpp>

Semaphore::Semaphore(int max_count, int current_count) : max_count(max_count), current_count(current_count) {

}

void Semaphore::acquire() {
	Scheduler::get().lock();

	if(current_count < max_count) {
		current_count++;
	} else {
		waiting_tasks.push_back(Scheduler::get().get_current_task());
		Scheduler::get().block(TaskState::WAITING);
	}

	Scheduler::get().unlock();
}

void Semaphore::release() {
	Scheduler::get().lock();

	if(waiting_tasks.size() > 0) {
		Task* task = waiting_tasks[0];
		waiting_tasks.erase(0);
		Scheduler::get().unblock(task);
	} else {
		current_count--;
	}

	Scheduler::get().unlock();
}