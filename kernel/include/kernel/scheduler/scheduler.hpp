#pragma once
#include <kernel/scheduler/task.hpp>
#include <kernel/datastr/cqueue.hpp>
#define TIME_QUANTUM_MS 20

class Scheduler {
	public:
		static Scheduler &get();
		void append_task(Task* task);
		void schedule();

		void preemptive_scheduling(int ellapsed_ms);
	private:
		Scheduler(){};
		Vector<Task*> tasks;
		Task** current_task;
		size_t choose_task();

		int time_slice = TIME_QUANTUM_MS;

		bool first_time = true;
		void *ksptr;
};