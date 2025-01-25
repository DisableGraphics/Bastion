#include <kernel/scheduler/scheduler.hpp>
#include <kernel/kernel/panic.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/kernel/log.hpp>
#include <kernel/assembly/inlineasm.h>

#define TIME_SLICE_LENGTH  20000000 // 20 MS

#define INIT 0
#define SCHEDULE 1
#define EXIT_TASK 2

extern "C" void switch_task(Task** current_thread, Task *next_thread);

extern "C" void task_startup(void) {
	Scheduler::get().unlock();
}

Scheduler::Scheduler() {

}

Scheduler& Scheduler::get() {
	static Scheduler instance;
	return instance;
}

void Scheduler::run() {
	init = true;
}

void Scheduler::append_task(Task* task) {
	static bool first_time = true;
	tasks.push_back(task);
	if(first_time) {
		current_task = &task;
		first_time = false;
	}
}

void Scheduler::schedule() {
	if(!init) return;
	//log(INFO, "Finished quantum for task %p", (*current_task)->id);
	size_t next = choose_task();
	lock();

	if (next == -1) {
		log(INFO, "No tasks available, running idle task");
		switch_task(current_task, tasks[0]);
		return;
	}
	log(INFO, "Changing tasks from %p to %p", (*current_task)->id, tasks[next]->id);
	switch_task(current_task, tasks[next]);
}

void Scheduler::sleep(unsigned millis) {
	(*current_task)->time_until_wake = millis;
	(*current_task)->status = TaskState::SLEEPING;
	schedule();
}

void Scheduler::preemptive_scheduling() {
	time_slice -= ms_clock_tick;
	if(time_slice < 0) {
		time_slice = TIME_QUANTUM_MS;
		schedule();
	}
}

void Scheduler::handle_sleeping_tasks() {
	for(size_t i = 1; i < tasks.size(); i++) {
		if(tasks[i]->status == TaskState::SLEEPING) {
			tasks[i]->time_until_wake -= ms_clock_tick;
			if(tasks[i]->time_until_wake <= 0) {
				tasks[i]->status = TaskState::RUNNING;
			}
		}

		if(tasks[i]->status == TaskState::TERMINATED) {
			log(INFO, "Task %d terminated", tasks[i]->id);
			Task* task = tasks[i];
			tasks.erase(i);
			delete task;
		}
	}
}

size_t Scheduler::choose_task() {
	for(size_t i = 1; i < tasks.size(); i++)
	{
		if (tasks[i]->status == TaskState::RUNNING) {
			// Move task to the back
			Task *task = tasks[i];
			tasks.erase(i);
			tasks.push_back(task);
			return tasks.size() - 1;
		}
	}

	return -1;
}

void Scheduler::lock() {
	nlock++;
	IDT::disable_interrupts();
}

void Scheduler::unlock() {
	if(nlock > 0) nlock--;
	if(nlock == 0) IDT::enable_interrupts();
}

void Scheduler::set_clock_tick(int ms) {
	ms_clock_tick = ms;
}

void Scheduler::block(TaskState reason) {
	(*current_task)->status = reason;
	schedule();
}

void Scheduler::unblock(Task* task) {
	log(INFO, "Task %d has been unlocked", task->id);
	task->status = TaskState::RUNNING;
}

void Scheduler::terminate() {
	log(INFO, "Task %d requested termination", (*current_task)->id);
	(*current_task)->status = TaskState::TERMINATED;
	schedule();
}

Task* Scheduler::get_current_task() {
	return *current_task;
}