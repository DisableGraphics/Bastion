#include <kernel/scheduler/task.hpp>

struct UserTask : public Task {
	void* user_stack_top = nullptr;
	UserTask(void (*fn)(void*), void* args);
	UserTask(const UserTask&) = delete;
	UserTask(UserTask&&);

	UserTask& operator=(const UserTask&) = delete;
	UserTask& operator=(UserTask&&);
	~UserTask();
};