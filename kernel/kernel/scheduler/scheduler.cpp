#include <kernel/scheduler/scheduler.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/cpp/icxxabi.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __i386
#include <../arch/i386/scheduler/interface.hpp>
#endif

Task * current_task_TCB;

Scheduler& Scheduler::get() {
	static Scheduler instance;
	return instance;
}

void Scheduler::set_first_task(Task* task) {
	current_task_TCB = task;
	first_ready_to_run_task = task;
	last_ready_to_run_task = task;
	printf("%p %p\n", first_ready_to_run_task, last_ready_to_run_task);
}

void Scheduler::append_task(Task* task) {
	last_ready_to_run_task->next_task = task;
	last_ready_to_run_task = last_ready_to_run_task->next_task;
	printf("%p %p\n", first_ready_to_run_task, last_ready_to_run_task);
}

void Scheduler::prepare_task(Task &task, void* eip) {
    uint32_t * stack = reinterpret_cast<uint32_t*>(task.esp);
    stack[-1] = reinterpret_cast<uint32_t>(eip);
    task.esp -= 5 * sizeof(uint32_t);
	task.esp0 = task.esp;
	task.state = TaskState::READY;
}

void Scheduler::lock() {
	#ifndef SMP
	lockn++;
	IDT::disable_interrupts();
	#endif
}

void Scheduler::unlock() {
	#ifndef SMP
	if(lockn > 0)
		lockn--;
	if(lockn == 0) {
		IDT::enable_interrupts();
	}
	#endif
}

void Scheduler::schedule() {
	if(first_ready_to_run_task != nullptr) {
        Task * task = first_ready_to_run_task;
        first_ready_to_run_task = task->next_task;
        switch_to_task(task);
    }
}

void Scheduler::block_task(TaskState state) {
	lock();
    current_task_TCB->state = state;
    schedule();
    unlock();
}

void Scheduler::unblock_task(Task * task) {
	lock();
    if(first_ready_to_run_task == nullptr) {
        // Only one task was running before, so pre-empt
        switch_to_task(task);
    } else {
        // There's at least one task on the "ready to run" queue already, so don't pre-empt
        last_ready_to_run_task->next_task = task;
        last_ready_to_run_task = task;
    }
    unlock();
}

void Scheduler::sleep(uint32_t millis) {
	uint32_t thandle = PIT::get().alloc_timer();
	serial_printf("hey\n");
	uint32_t *args = reinterpret_cast<uint32_t*>(kcalloc(2, sizeof(uint32_t)));
	serial_printf("hey hey %p\n", args);
	PIT::get().timer_callback(thandle, millis, [](void* args) {
		serial_printf("hey hey hey hey\n");
		Scheduler::get().lock();
		
		uint32_t *arg = reinterpret_cast<uint32_t*>(args);
		auto thandle = arg[0];
		Task * task = reinterpret_cast<Task*>(arg[1]);

		Scheduler::get().unblock_task(task);

		delete[] arg;
		
		PIT::get().clear_callback(thandle);
		Scheduler::get().unlock();
	}, reinterpret_cast<void*>(args));
	serial_printf("hey hey hey\n");
	block_task(TaskState::WAITING);
}