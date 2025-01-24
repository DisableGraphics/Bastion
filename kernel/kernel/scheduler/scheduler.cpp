#include <kernel/scheduler/scheduler.hpp>
#include <kernel/kernel/panic.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/kernel/log.hpp>

#define TIME_SLICE_LENGTH  20000000 // 20 MS

#define INIT 0
#define SCHEDULE 1
#define EXIT_TASK 2

extern "C" void switch_task(Task** current_thread, Task *next_thread);

extern "C" void task_startup(void) {
	//Scheduler::get().unlock();
}

Scheduler& Scheduler::get() {
	static Scheduler instance;
	return instance;
}

void Scheduler::append_task(Task* task) {
	tasks.push_back(task);
	current_task = &task;
}

void Scheduler::schedule() {
	size_t next = choose_task();

	if (next == -1) {
		return;
	}
	log(INFO, "Changing tasks from %p to %p", current_task, tasks[next]);
	switch_task(current_task, tasks[next]);
}

void Scheduler::preemptive_scheduling(int ellapsed_ms) {
	time_slice -= ellapsed_ms;
	if(time_slice < 0) {
		time_slice = TIME_QUANTUM_MS;
		schedule();
	}
}

size_t Scheduler::choose_task() {
	for(size_t i = 0; i < tasks.size(); i++)
	{
		if (tasks[i]->status == TaskState::RUNNING || tasks[i]->status == TaskState::CREATED) {
			// Put task in the first position
			Task *task = tasks[i];
			tasks.erase(i);
			tasks.push_back(task);
			// Return its position which should be the last one
			return tasks.size() - 1;
		}
	}

	return -1;
}