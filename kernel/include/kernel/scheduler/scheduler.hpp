#pragma once
#include <kernel/scheduler/task.hpp>
#include <kernel/datastr/cqueue.hpp>
#define TIME_QUANTUM_MS 20

class Scheduler {
	public:
		static Scheduler &get();
		void run();
		void append_task(Task* task);
		void schedule();
		void sleep(unsigned millis);

		void handle_sleeping_tasks();
		void preemptive_scheduling();

		void set_clock_tick(int ms);

		void lock();
		void unlock();
	private:
		Scheduler();
		Vector<Task*> tasks;
		Task* idle_task;
		Task** current_task;
		size_t choose_task();

		int time_slice = TIME_QUANTUM_MS;
		int nlock = 0;
		volatile bool init = false;
		int ms_clock_tick = 0;
};