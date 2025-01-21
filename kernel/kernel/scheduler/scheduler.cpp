#include <kernel/scheduler/scheduler.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/cpp/icxxabi.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/kernel/log.hpp>

#ifdef __i386
#include <../arch/i386/scheduler/interface.hpp>
#endif

#define TIME_SLICE_LENGTH  20000000 // 20 MS

Task * current_task_TCB;
int postpone_task_switches_counter = 0;
bool task_switches_postponed_flag = false;

extern "C" void task_startup(void) {
	Scheduler::get().unlock();
}

Scheduler& Scheduler::get() {
	static Scheduler instance;
	return instance;
}

void Scheduler::set_first_task(Task* task) {
	current_task_TCB = task;
	first_ready_to_run_task = task;
	last_ready_to_run_task = task;
	idle_task = task;
	printf("%p %p\n", first_ready_to_run_task, last_ready_to_run_task);
}

void Scheduler::append_task(Task* task) {
	last_ready_to_run_task->next_task = task;
	last_ready_to_run_task = last_ready_to_run_task->next_task;
	last_ready_to_run_task->next_task = first_ready_to_run_task->next_task;
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
	postpone_task_switches_counter++;
	IDT::disable_interrupts();
	#endif
}

void Scheduler::unlock() {
	#ifndef SMP
	if(postpone_task_switches_counter > 0)
		postpone_task_switches_counter--;
	if(postpone_task_switches_counter == 0) {
        if(task_switches_postponed_flag != 0) {
            task_switches_postponed_flag = 0;
            schedule();
        }
    }
	if(lockn > 0)
		lockn--;
	if(lockn == 0) {
		IDT::enable_interrupts();
	}
	#endif
}

void Scheduler::schedule() {
	if(postpone_task_switches_counter != 0) {
        task_switches_postponed_flag = 1;
        return;
    }
    if(first_ready_to_run_task != NULL) {
        Task * task = first_ready_to_run_task;
        first_ready_to_run_task = task->next_task;
        if(task == idle_task) {
            // Try to find an alternative to prevent the idle task getting CPU time
            if( first_ready_to_run_task != NULL) {
                // Idle task was selected but other task's are "ready to run"
                task = first_ready_to_run_task;
                idle_task->next_task = task->next_task;
                first_ready_to_run_task = idle_task;
            } else if(current_task_TCB->state == TaskState::RUNNING) {
                // No other tasks ready to run, but the currently running task wasn't blocked and can keep running
                return;
            } else {
                // No other options - the idle task is the only task that can be given CPU time
            }
        }
		log::log(log::INFO, "Switching to task %p\n", task);
        switch_to_task_wrapper(task);
    }

}

void Scheduler::block_task(TaskState state) {
	lock();
    current_task_TCB->state = state;
    schedule();
    unlock();
}

void Scheduler::unblock_task(Task* task) {
	lock();
    if(first_ready_to_run_task == nullptr || current_task_TCB == idle_task) {
        // Only one task was running before, so pre-empt
        switch_to_task_wrapper(task);
    } else {
        // There's at least one task on the "ready to run" queue already, so don't pre-empt
        last_ready_to_run_task->next_task = task;
        last_ready_to_run_task = task;
		task->next_task = first_ready_to_run_task->next_task;
    }
    unlock();
}

void Scheduler::sleep(uint32_t millis) {
	nano_sleep(millis * 1000000);
}

void Scheduler::nano_sleep(uint64_t nanoseconds) {
	nano_sleep_until(PIT::get().millis_accum() * 1000000 + nanoseconds);
}

void Scheduler::nano_sleep_until(uint64_t when) {
	lock();

    // Make sure "when" hasn't already occured
    if(when < (PIT::get().millis_accum() * 1000000)) {
        unlock();
        return;
    }

    // Set time when task should wake up
    current_task_TCB->sleep_expiry = when;

    // Add task to the start of the unsorted list of sleeping tasks
    current_task_TCB->next_task = sleeping_task_list;
    sleeping_task_list = current_task_TCB;

    unlock();

    // Find something else for the CPU to do

    block_task(TaskState::SLEEPING);
}

void Scheduler::handle_sleeping_tasks(uint64_t time_since_boot, uint64_t time_between_ticks) {
	Task * next_task;
    Task * this_task;

    lock();

    // Move everything from the sleeping task list into a temporary variable and make the sleeping task list empty

    next_task = sleeping_task_list;
    sleeping_task_list = nullptr;

    // For each task, wake it up or put it back on the sleeping task list

    while(next_task != nullptr) {
        this_task = next_task;
        next_task = this_task->next_task;

        if(this_task->sleep_expiry <= time_since_boot) {
            // Task needs to be woken up
            unblock_task(this_task);
        } else {
            // Task needs to be put back on the sleeping task list
            this_task->next_task = sleeping_task_list;
            sleeping_task_list = this_task;
        }
    }

	if(time_slice_remaining != 0) {
        // There is a time slice length
        if(time_slice_remaining <= time_between_ticks) {
            schedule();
        } else {
            time_slice_remaining -= time_between_ticks;
        }
    }


    // Done, unlock the scheduler (and do any postponed task switches!)
    unlock();

}

void Scheduler::switch_to_task_wrapper(Task *task) {
    if(postpone_task_switches_counter != 0) {
        task_switches_postponed_flag = 1;
        return;
    }

    if(current_task_TCB == NULL) {
        // Task unblocked and stopped us being idle, so only one task can be running
        time_slice_remaining = 0;
    } else if( (first_ready_to_run_task == NULL) && (current_task_TCB->state != TaskState::RUNNING) ) {
        // Currently running task blocked and the task we're switching to is the only task left
        time_slice_remaining = 0;
    } else {
        // More than one task wants the CPU, so set a time slice length
        time_slice_remaining = TIME_SLICE_LENGTH;
    }
    switch_to_task(task);
}
