#pragma once
#include "task_state.hpp"
#include <stdint.h>

struct Task {
	uint32_t esp;
	uint32_t cr3;
	uint32_t esp0;
	Task * next_task = nullptr;
	TaskState state;
};