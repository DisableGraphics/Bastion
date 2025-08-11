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
		self->user_space = true;
		self->esp = self->user_esp;
		jump_usermode(self->fn, self->user_esp);
	}
}

UserTask::UserTask(void (*fn)(void*), void* args) {
	this->fn = fn;
	kernel_stack_top = kcalloc(KERNEL_STACK_SIZE, 1);
	
	// Reserve space for registers + other things
	esp = reinterpret_cast<uint32_t>(reinterpret_cast<uintptr_t>(kernel_stack_top) + KERNEL_STACK_SIZE);
	// 4 registers + function + finish function + arguments
	// 7 4-byte elements
	void** stack = reinterpret_cast<void**>(esp);
	stack = stack - 8;
	stack[5] = reinterpret_cast<void*>(startup_usertask);
	stack[6] = reinterpret_cast<void*>(finish);
	stack[7] = reinterpret_cast<void*>(this);
	esp = reinterpret_cast<uint32_t>(stack);
	esp0 = esp;
	this->id = newid();
	log(INFO, "ESP: %p", esp);
	for(size_t i = 0; i < 8; i++)
		log(INFO, "%d: %p", i, *(reinterpret_cast<void**>(esp)+i));
	setup_pages(fn, args);
	log(INFO, "User Task created: %p %p %p", kernel_stack_top, esp, user_esp);
}

void UserTask::setup_pages(void (*fn)(void*), void* args) {
	page_directory = reinterpret_cast<void**>(MemoryManager::get().alloc_pages(1));
	if(!page_directory) kn::panic("No space for user process");
	memcpy(page_directory, PagingManager::get().get_page_directory(), PAGE_SIZE);
	for(size_t i = 0; i < PAGE_SIZE/sizeof(void*); i++) {
		log(INFO, "PD %p: %p", i * REGION_SIZE, page_directory[i]);
	}
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

	uint32_t* sptr = reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(user_stack_pages.back()) + HIGHER_HALF_OFFSET - 32);
	sptr[5] = reinterpret_cast<uint32_t>(fn);
	sptr[6] = reinterpret_cast<uint32_t>(finish);
	sptr[7] = reinterpret_cast<uint32_t>(args);

	sptr += 4;

	user_esp = reinterpret_cast<size_t>(user_stack_virtaddr - 12);
	log(INFO, "User ESP: %p", user_esp);
	for(size_t i = 0; i < 4; i++)
		log(INFO, "User: %p: %p", user_esp + (i*4), *(reinterpret_cast<void**>(sptr)+i));
}

UserTask::~UserTask() {
	log(INFO, "Destroyed UserTask");
	kfree(stack_page_table);
	for(size_t i = 0; i < user_stack_pages.size(); i++) {
		MemoryManager::get().free_pages(user_stack_pages[i], 1);
	}
}
