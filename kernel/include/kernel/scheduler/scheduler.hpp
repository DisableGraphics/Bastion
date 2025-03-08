#pragma once
#include <kernel/scheduler/task.hpp>
#include <kernel/datastr/vector.hpp>
#define TIME_QUANTUM_MS 20

class Scheduler {
	public:
		/**
			\brief Get singleton instance of the scheduler.
		 */
		static Scheduler &get();
		/**
			\brief Start scheduling.
		 */
		void run();
		/**
			\brief Add a task to the list of tasks to be run.
			\param task A heap pointer to the task
			\note First task to be appended must be the idle task.
			\details The task is added as the last task, so it may take a while to run.
			The pointer to the task must be in the heap as resources are freed automatically
			since terminate() calls the delete operator.
		 */
		void append_task(Task* task);
		/**
			\brief Schedule another task to run.
			\details Works the same as yield() in the world of cooperative multitasking.
		 */
		void schedule();
		/**
			\brief Sleep for at least millis milliseconds.
			\param millis The minimum amount of time this task will sleep in milliseconds.
			\warning The scheduler can't guarantee the exact time the task will wake up.
			However, it can guarantee that the task will sleep for at least the specified
			amount of time.
		 */
		void sleep(unsigned millis);
		/**
			\brief Block task for a reason (e.g waiting for IO, sleeping)
			\param reason The reason the task is sleeping
			\details All the reasons a task can be asleep is in the TaskState enum.
			You can technically call block(TaskState::RUNNING) but it's slightly cheaper
			to just call schedule() directly as both have the same effect.
		 */
		void block(TaskState reason);
		/**
			\brief Unblock task to be ready for scheduling
			\param task The pointer to the task to be unblocked
		 */
		void unblock(Task* task);
		/**
			\brief Terminate task and free all its resources.
			\details Calls operator delete so task must be in heap
		 */
		void terminate();
		/**
			\brief Get a pointer to the task that is currently running.
		 */
		Task* get_current_task();

		/**
			\brief Handles sleeping tasks, updating their time + frees terminated tasks.
			Don't call this function in a place other than in a timer interrupt handler.
		 */
		void handle_sleeping_tasks();
		/**
			\brief Handles preemptive scheduling from the timer.
		 */
		void preemptive_scheduling();
		/**
			\brief Set lenght of clock tick in milliseconds for preemptive scheduling.
		 */
		void set_clock_tick(int ms);

		/**
			\brief Lock scheduler.
			For single core systems this just disables interrupts.
		 */
		void lock();
		/**
			\brief Unlock scheduler.
			For single core systems this just re-enables interrupts.
		 */
		void unlock();
	private:
		Scheduler();
		// Tasks queue
		Vector<Task*> tasks;
		// Current task running. Double pointer, changes when calling switch_task
		Task** current_task = nullptr;
		/**
			\brief Choose available task
			\return -1 if no task is available, position in the tasks queue otherwise
		 */
		size_t choose_task();
		// Time slice of the currently running task
		int time_slice = TIME_QUANTUM_MS;
		// Number of times lock() has been called
		int nlock = 0;
		// Whether run() has been called
		bool init = false;
		// Length of clock tick of the timer
		int ms_clock_tick = 0;
		// Schedule early (when preempting the idle task)
		bool early_sched = false;
};