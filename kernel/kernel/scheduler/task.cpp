#include "liballoc/liballoc.h"
#include <kernel/scheduler/task.hpp>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/kernel/log.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/scheduler/scheduler.hpp>

int newid() {
	static int id = 0;
	return id++;
}

void Task::finish() {
	log(INFO, "Task::finish() called");
	Scheduler::get().terminate();
}
