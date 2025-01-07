#include <kernel/scheduler/scheduler.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>

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

void Scheduler::start() {
	// Schedule the first timer interrupt for preemption
	do_after(TIME_QUANTUM_MS, [](void *) { preempt(); });
	idle();
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
	printf("Moved to task %d\n", next_task);
}

void Scheduler::do_after(uint32_t millis, void (*fn)(void*)) {
	auto &pit = PIT::get();
	pit.timer_callback(timer_handle, millis, fn, NULL);
}

void Scheduler::context_switch(uint32_t **old_sp, uint32_t *new_sp, void (*fn)()) {
	jump_fn(old_sp, new_sp, fn, 1, 2, 0);
}

void Scheduler::idle() {
	while (true) {
		// Idle loop (e.g., low-power mode)
		halt();
	}
}