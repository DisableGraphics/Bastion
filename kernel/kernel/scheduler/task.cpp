#include "liballoc/liballoc.h"
#include <kernel/scheduler/task.hpp>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/kernel/log.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/scheduler/scheduler.hpp>

static int idn = 0;

Task::Task(void (*fn)(void*), void* args) : fn(fn) {
	stack_bottom = kcalloc(KERNEL_STACK_SIZE, 1);
	// Reserve space for registers + other things
	esp = reinterpret_cast<uint32_t>(reinterpret_cast<uintptr_t>(stack_bottom) + KERNEL_STACK_SIZE);
	log(INFO, "Task created: %p %p", stack_bottom, esp);

	memset(stack_bottom, 0, KERNEL_STACK_SIZE);

	// 4 registers + function + finish function + arguments
	// 7 4-byte elements
	void** stack = reinterpret_cast<void**>(esp);
	stack = stack - 8;
	stack[5] = reinterpret_cast<void*>(fn);
	stack[6] = reinterpret_cast<void*>(finish);
	stack[7] = reinterpret_cast<void*>(args);
	esp = reinterpret_cast<uint32_t>(stack);
	esp0 = esp;
	cr3 = read_cr3();
	this->id = idn++;
	log(INFO, "ESP: %p", esp);
	for(size_t i = 0; i < 8; i++)
		log(INFO, "%d: %p", i, *(reinterpret_cast<void**>(esp)+i));
}

Task::Task(Task&& other) {
	memcpy(this, &other, sizeof(Task));
	other.stack_bottom = nullptr;
}

Task::Task(const Task& other) {
	memcpy(this, &other, sizeof(Task));
}

Task::~Task() {
	log(INFO, "Task destroyed: %p %p", stack_bottom, esp0);
	kfree(stack_bottom);
}

Task& Task::operator=(const Task& other) {
	memcpy(this, &other, sizeof(Task));
	return *this;
}

void Task::finish() {
	log(INFO, "Task::finish() called");
	Scheduler::get().terminate();
}