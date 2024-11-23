#pragma once
#include <stdint.h>
#include <multiboot/multiboot.h>
#include <kernel/page.hpp>

struct invalid_address {
	void * begin;
	void * end;
};

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
		uint8_t * alloc_bitmap();
		size_t memsize = 0;
		typedef uint8_t bitmap_t;
		bitmap_t *pages_bitmap = nullptr;
		size_t bitmap_size = 0;
		size_t bitmap_size_pages = 0;

		// 32 invalid addresses blocks
		invalid_address invalid_addresses[32];
		size_t indaddress_size = 0;
		
		MemoryManager() {};
};