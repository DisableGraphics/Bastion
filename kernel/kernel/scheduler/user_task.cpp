#include <kernel/scheduler/user_task.hpp>

extern "C" void jump_usermode(void (*fn)(void*));

UserTask::UserTask(void (*fn)(void*), void* args) : Task(fn, args) {
	
}

void UserTask::startup() {
	if(!user_space) {
		user_space = true;
		jump_usermode(fn);
	}
}