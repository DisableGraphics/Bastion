#include <kernel/scheduler/task.hpp>

struct UserTask : public Task {
	void* user_stack_top = nullptr;
	UserTask(void (*fn)(void*), void* args);
	UserTask(const UserTask&) = delete;
	UserTask(UserTask&&);

	void startup() override;

	UserTask& operator=(const UserTask&) = delete;
	UserTask& operator=(UserTask&&);
	~UserTask();

	bool user_space = false;
};