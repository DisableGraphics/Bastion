#pragma once
#include <stdint.h>
#include <multiboot/multiboot.h>
#include <kernel/page.hpp>

/**
	\brief Memory manager for the kernel
	Implemented as a singleton
	Is the one that does and keeps track of memory allocations
 */
class MemoryManager {
	public:
		static MemoryManager &get();
		void init(multiboot_info_t* mbd, unsigned int magic);
	private:
		void add_to_pages_bitmap(multiboot_memory_map_t* mmmt);
		size_t memsize = 0;
		uint8_t *pages_bitmap = nullptr;
		MemoryManager() {};
};