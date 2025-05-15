#pragma once
#include <kernel/scheduler/task.hpp>

struct KernelTask : public Task {
	public:
		KernelTask(void (*fn)(void*), void* args);
		KernelTask(const KernelTask&) = delete;
		KernelTask(KernelTask&&);

		KernelTask& operator=(const KernelTask&) = delete;
		KernelTask& operator=(KernelTask&&);
		~KernelTask();
	private:
		void* stack_top;
		void steal(KernelTask&& other);
		void clean();
};