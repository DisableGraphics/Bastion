#pragma once
#include "task_state.hpp"
#include <stdint.h>

#define KERNEL_STACK_SIZE 16*1024 // 16 KiB

struct Task {
	uint32_t esp0, cr3, esp;

	void* stack_bottom;
	TaskState status;

	static void finish();

	void (*fn)(void*);

	Task(void (*fn)(void*), void* args);
	Task(const Task&);
	Task(Task&&);

	Task& operator=(const Task&);

	~Task();
};