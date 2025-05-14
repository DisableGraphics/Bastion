#include <kernel/scheduler/user_task.hpp>
#include <kernel/kernel/log.hpp>

extern "C" void jump_usermode(void (*fn)(void*));

void startup_user(void* u) {
	UserTask* self = reinterpret_cast<UserTask*>(u);
	if(!self->user_space) {
		log(INFO, "UserTask::startup()");
		self->user_space = true;
		jump_usermode(self->fn);
	}
}

UserTask::UserTask(void (*fn)(void*), void* args) : Task(fn, args) {
	this->startup = startup_user;
	this->startuparg = this;
}

