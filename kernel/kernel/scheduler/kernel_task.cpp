#include "kernel/cpp/move.hpp"
#include <kernel/scheduler/kernel_task.hpp>
#include <kernel/kernel/log.hpp>
#include <stdlib.h>
#include <string.h>
#include <kernel/assembly/inlineasm.h>

KernelTask::KernelTask(void (*fn)(void*), void* args) {
	this->fn = fn;
	stack_top = kcalloc(KERNEL_STACK_SIZE, 1);
	// Reserve space for registers + other things
	esp = reinterpret_cast<uint32_t>(reinterpret_cast<uintptr_t>(stack_top) + KERNEL_STACK_SIZE);
	log(INFO, "Task created: %p %p", stack_top, esp);

	memset(stack_top, 0, KERNEL_STACK_SIZE);

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
	this->id = newid();
	log(INFO, "ESP: %p", esp);
	for(size_t i = 0; i < 8; i++)
		log(INFO, "%d: %p", i, *(reinterpret_cast<void**>(esp)+i));
}

KernelTask::~KernelTask() {
	clean();
}

KernelTask::KernelTask(KernelTask&& other) {
	if(this != &other) {
		clean();
		steal(move(other));
	}
}

KernelTask& KernelTask::operator=(KernelTask&& other) {
	if(this != &other) {
		clean();
		steal(move(other));user_stack_pages.back()) + HIGHER_HALF_OFFSET
	}
	return *this;
}

void KernelTask::steal(KernelTask&& other) {
	this->cr3 = other.cr3;
	this->esp = other.esp;
	this->esp0 = other.esp0;
	this->fn = other.fn;
	this->id = other.id;
	this->stack_top = other.stack_top;
	this->status = other.status;
	other.stack_top = nullptr;
}

void KernelTask::clean() {
	if (stack_top)
		kfree(stack_top);
}

