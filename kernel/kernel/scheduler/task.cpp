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
	*(--stack_pointer) = reinterpret_cast<uint32_t>(entry_point); // Entry point
	*(--stack_pointer) = get_eflags();
	for (int i = 0; i < 8; i++) { // Push placeholders for general-purpose registers
		*(--stack_pointer) = 0;
	}
}

void Task::task_end() {
	halt();
}