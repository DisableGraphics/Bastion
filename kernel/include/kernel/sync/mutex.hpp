#pragma once
#include <kernel/scheduler/task.hpp>
#include <kernel/datastr/vector.hpp>

/**
	\brief Mutex class for tasks spawned using the scheduler.
	\note This mutex can't guarantee inmediate wakeup but it 
	can guarantee that the task won't wake up in the wrong moment
 */
class Mutex {
	public:
		/**
			\brief Constructor.
			\note The mutex starts unlocked (as it should).
		 */
		Mutex();
		/**
			\brief Locks mutex. Task goes to sleep if mutex is already locked
		 */
		void lock();
		/**
			\brief Unlocks mutex. Wakes one task if there are tasks waiting
		 */
		void unlock();
	private:
		// Whether the mutex is locked or not
		volatile bool locked;
		// Tasks waiting for unlock()
		Vector<Task*> waiting_tasks;
};