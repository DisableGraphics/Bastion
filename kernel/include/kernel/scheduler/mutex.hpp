#pragma once
#include <kernel/scheduler/task.hpp>
#include <kernel/datastr/vector.hpp>

class Mutex {
	public:
		Mutex();
		void lock();
		void unlock();
	private:
		bool locked;
		Vector<Task*> waiting_tasks;
};