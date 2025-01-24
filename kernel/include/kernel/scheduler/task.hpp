#pragma once
#include "task_state.hpp"
#include <stdint.h>

#define KERNEL_STACK_SIZE 16*1024 // 16 KiB

struct Task {
	uint32_t esp, cr3, esp0;

	void* stack_bottom = nullptr;
	TaskState status = TaskState::RUNNING;

	int time_until_wake = 0;
	int id;

	static void finish();

	void (*fn)(void*);

	Task(void (*fn)(void*), void* args);
	Task(const Task&);
	Task(Task&&);

	Task& operator=(const Task&);

	~Task();
};