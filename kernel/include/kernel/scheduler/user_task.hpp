#pragma once
#include "kernel/datastr/vector.hpp"
#include <kernel/scheduler/task.hpp>
#include <stddef.h>

struct UserTask : public Task {
	public:
		UserTask(void (*fn)(void*), void* args);
		UserTask(const UserTask&) = delete;
		UserTask(UserTask&&);

		UserTask& operator=(const UserTask&) = delete;
		UserTask& operator=(UserTask&&);
		~UserTask();
		bool user_space = false;
	private:
		void* kernel_stack_top = nullptr;
		void* user_stack_top = nullptr;
		void** page_directory = nullptr;
		void** stack_page_table = nullptr;
		
		size_t user_stack_size = 0;
		Vector<void*> user_stack_pages;
		void steal(UserTask&& other);
		void clean();
		void setup_pages(void (*fn)(void*), void* args);
};