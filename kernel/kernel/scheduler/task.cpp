#include "liballoc/liballoc.h"
#include <kernel/scheduler/task.hpp>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/kernel/log.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/scheduler/scheduler.hpp>
#include <kernel/memory/page.hpp>


Task::Task() {
	size_t useraddr = reinterpret_cast<size_t>(Task::finish);
	useraddr = useraddr & 0xFFFFF000;
	printf("fn_ptr: %p, useradd: %p\n", Task::finish, useraddr);
	
	// Map Task::finish() as user-readable
	PagingManager::get().map_page(reinterpret_cast<void*>(useraddr - HIGHER_HALF_OFFSET),
		reinterpret_cast<void*>(useraddr),
		USER | PRESENT);
	PagingManager::get().set_global_options(reinterpret_cast<void*>(useraddr), USER | READ_WRITE | PRESENT);
	useraddr += PAGE_SIZE;
	PagingManager::get().map_page(reinterpret_cast<void*>(useraddr - HIGHER_HALF_OFFSET),
		reinterpret_cast<void*>(useraddr),
		USER | PRESENT);
	PagingManager::get().set_global_options(reinterpret_cast<void*>(useraddr), USER | READ_WRITE | PRESENT);
}

int newid() {
	static int id = 0;
	return id++;
}

void Task::finish() {
	log(INFO, "Task::finish() called");
	Scheduler::get().terminate();
}
