#include <kernel/scheduler/user_task.hpp>

UserTask::UserTask(void (*fn)(void*), void* args) : Task(fn, args) {
	
}