#pragma once
#include <stdint.h>
#include <multiboot/multiboot2.h>
#include <kernel/memory/page.hpp>

/**
	\brief Already used regions of memory.
	Either by the hardware or the kernel.
 */
struct used_region {
	// Beginning of the region
	void * begin;
	// End of the region
	void * end;
	/**
		\brief Wether the address addr is contained in this used region.
		\param addr The address to check.
		\return true if addr is in this region, false otherwise.
	 */
	bool contains(void *addr) {
		uintptr_t addr_ptr = reinterpret_cast<uintptr_t>(addr);
		return reinterpret_cast<uintptr_t>(begin) >= addr_ptr && reinterpret_cast<uintptr_t>(end) <= addr_ptr;
	}
};

/**
	\brief Memory manager for the kernel.
	Implemented as a singleton.
	Is the one that does and keeps track of memory allocations.
 */
class MemoryManager {
	public:
		static MemoryManager &get();
		void init(const multiboot_tag_mmap* mbd, const void* last_module_addr);
		/**
			\brief Allocate pages continuous pages.
			\param pages The number of pages to allocate.
			\return The address of the first page or NULL
			if there isn't any memory left		 */
		void * alloc_pages(size_t pages, size_t map_flags = READ_WRITE);
		void* alloc_pages_debug(size_t pages, size_t map_flags);
		void dump_recent_allocs();
		/**
			\brief Free pages continuous pages.
			\param start Address of the first page.
			\param pages Number of pages to free.
		 */
		void free_pages(void *start, size_t pages);
		void free_pages_debug(void *start, size_t pages);
		/**
			\brief Get used regions.
		 */
		used_region * get_used_regions();
		/**
			\brief Get number of used regions.
		 */
		size_t get_used_regions_size();
	private:
		typedef uint32_t bitmap_t;
		/**
			\brief Initialise the page bitmap.
			This explodes if your computer has more than
			128 TiB of RAM.
		 */
		bitmap_t * alloc_bitmap();
		/**
			\brief Allocate the page tables vector.
		 */
		void alloc_pagevec();
		/**
			\brief memsize: size of all memory.
			real_memsize: size of available memory.
		 */
		size_t memsize = 0, real_memsize = 0;
		/**
			\brief Pages bitmap. Contains wether a page
			has been allocated (1) or not (0).
		 */
		bitmap_t *pages_bitmap = nullptr;
		/**
			\brief bitmap_size: size of the bitmap in bytes.
			bitmap_size_pages: size of the bitmap in pages.
		 */
		size_t bitmap_size_bytes = 0;
		size_t bitmap_size_pages = 0;
		// Size of the page tables vector in bytes
		size_t pagevec_size = 0;

		void mark_as_used(void* start, void* end);
		const void * last_module_addr;

		// 32 used region blocks. Should be enough.
		used_region used_regions[32];
		size_t ureg_size = 0;
		constexpr static size_t bits_per_bitmap_entry = sizeof(bitmap_t) * BITS_PER_BYTE;
		
		MemoryManager() {
			memset(used_regions, 0, sizeof(used_regions));
		};

		typedef uintptr_t physaddr_t;
		typedef uintptr_t virtaddr_t;
};