#include "kernel/memory/page.hpp"
#include <kernel/scheduler/user_task.hpp>
#include <kernel/kernel/log.hpp>
#include <kernel/memory/mmanager.hpp>
#include <stdlib.h>
#include <kernel/kernel/panic.hpp>

constexpr size_t INITIAL_STACK_PAGES = 4;
constexpr size_t user_stack_virtaddr = HIGHER_HALF_OFFSET - 16;

extern "C" [[gnu::noreturn]] void jump_usermode(void (*fn)(void*), size_t esp);

void startup_usertask(void* s) {
	UserTask* self = reinterpret_cast<UserTask*>(s);
	if(!self->user_space) {
		log(INFO, "UserTask::startup()");
		self->user_space = true;
		log(INFO, "Jumping to %p", self->fn);
		jump_usermode(self->fn, self->user_esp);
		log(INFO, "SHOULD NEVER BE HERE");
	}
}

UserTask::UserTask(void (*fn)(void*), void* args) {
	kernel_stack_top = kmalloc(KERNEL_STACK_SIZE);
	esp0 = reinterpret_cast<size_t>(kernel_stack_top) + KERNEL_STACK_SIZE;
	setup_pages(fn, args);
	this->startup = startup_usertask;
	this->startupargs = this;
	id = newid();
	this->fn = fn;
}

void UserTask::setup_pages(void (*fn)(void*), void* args) {
	page_directory = reinterpret_cast<void**>(MemoryManager::get().alloc_pages(1));
	if(!page_directory) kn::panic("No space for user process");
	memcpy(page_directory, PagingManager::get().get_page_directory(), PAGE_SIZE);
	cr3 = reinterpret_cast<size_t>(page_directory);
	cr3 -= HIGHER_HALF_OFFSET;

	stack_page_table = reinterpret_cast<void**>(MemoryManager::get().alloc_pages(1));
	memset(stack_page_table, 0, PAGE_SIZE);
	
	// Setup stack
	user_stack_size = INITIAL_STACK_PAGES;
	
	for(size_t i = 0; i < INITIAL_STACK_PAGES; i++) {
		const size_t addr = user_stack_virtaddr - (i*PAGE_SIZE);
		log(INFO, "Address: %p", addr);
		page_directory[addr >> 22] = reinterpret_cast<void*>((reinterpret_cast<size_t>(stack_page_table) - HIGHER_HALF_OFFSET) | USER | READ_WRITE | PRESENT);
		size_t page = reinterpret_cast<size_t>(MemoryManager::get().alloc_pages(1));
		memset(reinterpret_cast<void*>(page), 0, PAGE_SIZE);
		user_stack_pages.push_back(reinterpret_cast<void*>(page - HIGHER_HALF_OFFSET));
		stack_page_table[addr>> 12 & 0x03FF] = reinterpret_cast<void*>(
			reinterpret_cast<size_t>(user_stack_pages.back()) | USER | READ_WRITE | PRESENT
		);
	}
	user_stack_top = reinterpret_cast<void*>(reinterpret_cast<size_t>(user_stack_pages.back()) + HIGHER_HALF_OFFSET);

	uint32_t* sptr = reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(user_stack_pages.back()) + HIGHER_HALF_OFFSET);
	sptr -= 8;
	sptr[0] = 0x202;
	sptr[5] = reinterpret_cast<uint32_t>(fn);
	sptr[6] = reinterpret_cast<uint32_t>(finish);
	sptr[7] = reinterpret_cast<uint32_t>(args);

	esp = esp0 - 32;
	user_esp = reinterpret_cast<size_t>(user_stack_virtaddr);
	log(INFO, "ESP: %p", esp);
	for(size_t i = 0; i < 8; i++)
		log(INFO, "%p: %p", esp + (i*4), *(reinterpret_cast<void**>(sptr)+i));
}

UserTask::~UserTask() {
	log(INFO, "Destroyed UserTask");
	kfree(stack_page_table);
	for(size_t i = 0; i < user_stack_pages.size(); i++) {
		MemoryManager::get().free_pages(user_stack_pages[i], 1);
	}
}
