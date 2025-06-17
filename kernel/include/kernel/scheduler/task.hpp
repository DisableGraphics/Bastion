#pragma once
#include "task_state.hpp"
#include <stdint.h>

#define KERNEL_STACK_SIZE 16*1024 // 16 KiB

/**
	\brief Base task that the scheduler handles.
 */
struct Task {
	/// Current stack pointer
	uint32_t esp;
	/// Virtual memory register
	uint32_t cr3;
	/// Kernel stack pointer
	uint32_t esp0;
	/// Current status of the task
	TaskState status = TaskState::RUNNING;

	/// Time until task wakes if it is sleeping
	int time_until_wake = 0;
	/// ID of the task
	int id;

	/**
		\brief When a task finishes (e.g returns) it calls this function.
		This function just terminates the task.
	 */
	static void finish();

	Task();

	/**
		\brief Pointer to the function that the task will execute.
	 */
	void (*fn)(void*);
};

int newid();