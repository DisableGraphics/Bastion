#include <kernel/scheduler/scheduler.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/cpp/icxxabi.h>

#ifdef __i386
#include <../arch/i386/scheduler/interface.hpp>
#endif

Scheduler::Scheduler() : current_task(0), task_count(0) {
	timer_handle = PIT::get().alloc_timer();
}

Scheduler& Scheduler::get() {
	static Scheduler instance;
	return instance;
}

void Scheduler::create_task(void (*func)()) {
	if (task_count < MAX_TASKS) {
		tasks[task_count++].initialize(func);
	}
}

struct args {
	uint32_t handle;
	size_t id;
};

void Scheduler::sleep_task(size_t id, uint32_t millis) {
	tasks[id].state = TaskState::WAITING;
	auto &pit = PIT::get();
	auto hnd = pit.alloc_timer();

	struct args * ach = new struct args;
	ach->handle = hnd;
	ach->id = id;

	pit.timer_callback(hnd, millis, task_state_cb, reinterpret_cast<void*>(ach));
}

void Scheduler::sleep_task(uint32_t millis) {
	sleep_task(current_task, millis);
}

void Scheduler::start() {
	// Schedule the first timer interrupt for preemption
	do_after(TIME_QUANTUM_MS, [](void *) { preempt(); });
}

#include <stdio.h>

void Scheduler::preempt() {
	Scheduler &instance = Scheduler::get();
	// Perform a context switch to the next READY task
	size_t previous_task = instance.current_task;
	size_t next_task = (instance.current_task + 1) % instance.task_count;

	for (size_t i = 0; i < instance.task_count; i++) {
		if (instance.tasks[next_task].state == TaskState::READY) {
			serial_printf("Selected %d\n", next_task);
			break;
		}
		next_task = (next_task + 1) % instance.task_count;
	}
	if (instance.tasks[next_task].state == TaskState::READY) {
		instance.current_task = next_task;
		instance.do_after(TIME_QUANTUM_MS, [](void *) { preempt(); });
		instance.context_switch(&instance.tasks[previous_task].stack_pointer, instance.tasks[next_task].stack_pointer, instance.tasks[next_task].entry_point);
	}
}

void Scheduler::task_state_cb(void* args) {
	Scheduler &instance = Scheduler::get();
	struct args* task = reinterpret_cast<struct args*>(args);
	PIT::get().dealloc_timer(task->handle);
	instance.tasks[task->id].state = TaskState::READY;
	delete task;
}

void Scheduler::do_after(uint32_t millis, void (*fn)(void*), void *args) {
	auto &pit = PIT::get();
	pit.timer_callback(timer_handle, millis, fn, args);
}

void Scheduler::context_switch(uint32_t **old_sp, uint32_t *new_sp, void (*fn)()) {
	serial_printf("Ptr for task: %p\n", new_sp);
	jump_fn(old_sp, new_sp, fn, 1, 2, 0);
}

void Scheduler::idle() {
	while (true) {
		halt();
	}
}

void Scheduler::set_task_state(size_t taskid, TaskState state) {
	tasks[taskid].state = state;
}