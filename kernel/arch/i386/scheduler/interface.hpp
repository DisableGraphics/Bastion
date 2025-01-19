#include <kernel/scheduler/task.hpp>

extern "C" void switch_to_task(Task *next_thread);