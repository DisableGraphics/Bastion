#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/scheduler/task.hpp>

/**
	\brief Semaphore class for tasks spawned using the scheduler.
	\note This semaphore can't guarantee inmediate wakeup but it 
	can guarantee that the task won't wake up in the wrong moment
 */
class Semaphore {
	public:
		/**
			\brief Constructor.
			\param max_count Maximum number of acquire() calls before the semaphore locks.
			\param current_count Defaults to 0. Current number of acquire() calls.
		 */
		Semaphore(int max_count, int current_count = 0);
		/**
			\brief Acquire semaphore.
			\details If current_count >= max_count blocks the task.
			Changes current_count by adding one to it.
		 */
		void acquire();
		/**
			\brief Release semaphore.
			\details Unblocks one waiting task if there are sleeping tasks waiting.
		 */
		void release();
	private:
		// Max number of acquire calls before semaphore locks and current number of them
		int max_count, current_count;
		// Tasks that are waiting for this semaphore to unlock
		Vector<Task*> waiting_tasks;
};