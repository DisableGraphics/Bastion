#include <kernel/scheduler/task.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/scheduler/defs.hpp>
#include <stdlib.h>
#include <stdio.h>

Task::Task() : state(TaskState::FINISHED), stack_pointer(nullptr) {}

void Task::initialize(void (*entry_point)()) {
	state = TaskState::READY;
	kfree(stack);
	stack = reinterpret_cast<uint32_t*>(kcalloc(STACK_SIZE, sizeof(uint32_t)));
	stack_pointer = &stack[STACK_SIZE];

	*(--stack_pointer) = reinterpret_cast<uint32_t>(task_end);   // Return address
	this->entry_point = entry_point;
}

void Task::task_end() {
	halt();
}
