#pragma once
#include "task_state.hpp"
#include <stdint.h>

class Task {
	public:
		Task();

		void initialize(void (*entry_point)());

		TaskState state;                 // Current state of the task
		uint32_t *stack_pointer;         // Stack pointer for the task
		uint32_t* stack = nullptr;      // Fixed-size stack

	private:
		static void task_end();
};