#pragma once
#include <stdint.h>
#include <multiboot/multiboot.h>
#include <kernel/page.hpp>

/**
	\brief Already used regions of memory.
	Either by the hardware or the kernel.
 */
struct used_region {
	void * begin;
	void * end;
	bool contains(void *addr) {
		uintptr_t addr_ptr = reinterpret_cast<uintptr_t>(addr);
		return reinterpret_cast<uintptr_t>(begin) >= addr_ptr && reinterpret_cast<uintptr_t>(end) <= addr_ptr;
	}
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
		/**
			\brief Allocate pages continuous pages.
			\param pages The number of pages to allocate.
			\return The address of the first page or NULL
			if there isn't any memory left
		 */
		void * alloc_pages(size_t pages);
		/**
			\brief Free pages continuous pages.
			\param start Address of the first page.
			\param pages Number of pages to free.
		 */
		void free_pages(void *start, size_t pages);
	private:
		uint8_t * alloc_bitmap();

		size_t memsize = 0, real_memsize = 0;
		typedef uint8_t bitmap_t;
		bitmap_t *pages_bitmap = nullptr;
		size_t bitmap_size = 0;
		size_t bitmap_size_pages = 0;

		// 32 used regions blocks. Should be enough.
		used_region used_regions[32];
		size_t ureg_size = 0;
		
		MemoryManager() {};
};