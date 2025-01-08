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
		void sleep_task(size_t id, uint32_t millis);
		void sleep_task(uint32_t millis);

		static void preempt();
		void set_task_state(size_t taskid, TaskState state);

	private:
		Scheduler();
		size_t current_task;
		size_t task_count;
		uint32_t timer_handle;
		Task tasks[MAX_TASKS];

		static void task_state_cb(void* args);

		void do_after(uint32_t millis, void (*fn)(void*), void* args = nullptr);
		void context_switch(uint32_t **old_sp, uint32_t *new_sp, void (*fn)());
		void idle();
};