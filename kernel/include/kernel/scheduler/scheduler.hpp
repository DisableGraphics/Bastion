#include <stdint.h>
#include <stddef.h>
#include "task.hpp"

#define MAX_TASKS 16
#define TIME_QUANTUM_MS 20


class Scheduler {
	public:
		static Scheduler &get();
		void create_task(void (*func)());

		void start();

		static void preempt();

	private:
		Scheduler();
		size_t current_task;
		size_t task_count;
		uint32_t timer_handle;
		Task tasks[MAX_TASKS];

		void do_after(uint32_t millis, void (*fn)(void*));
		void context_switch(uint32_t **old_sp, uint32_t *new_sp);
		void idle();
};