#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/scheduler/task.hpp>

class Semaphore {
	public:
		Semaphore(int max_count, int current_count = 0);
		void acquire();
		void release();
	private:
		int max_count, current_count;
		Vector<Task*> waiting_tasks;
};