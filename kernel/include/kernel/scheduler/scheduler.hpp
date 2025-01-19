#pragma once
#include <kernel/scheduler/task.hpp>
#define TIME_QUANTUM_MS 20

class Scheduler {
	public:
		static Scheduler &get();
		void set_first_task(Task* task);
		void append_task(Task* task);
		void prepare_task(Task& task, void * eip);
		void block_task(TaskState state);
		void unblock_task(Task* task);

		void sleep(uint32_t millis);
		void nano_sleep(uint64_t nanoseconds);
		void nano_sleep_until(uint64_t when);

		void handle_sleeping_tasks(uint64_t time_since_boot, uint64_t time_between_ticks);

		void switch_to_task_wrapper(Task* task);

		void schedule();
		void lock();
		void unlock();
	private:
		uint32_t lockn = 0;
		Task* first_ready_to_run_task = nullptr;
		Task* last_ready_to_run_task = nullptr;

		Task* sleeping_task_list = nullptr;
		Task* idle_task = nullptr;

		uint64_t time_slice_remaining = 0;

		Scheduler(){};

};