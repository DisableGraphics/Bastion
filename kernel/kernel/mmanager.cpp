#include "multiboot/multiboot.h"
#include <stddef.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/memory/mmanager.hpp>
#include <kernel/kernel/panic.hpp>
#include <kernel/kernel/const.hpp>
#include <kernel/memory/page.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/kernel/log.hpp>

#ifdef __i386
#include "../arch/i386/defs/page/pagedef.h"
#include "../arch/i386/defs/sizes/max_addr_size.hpp"
#endif

extern uint32_t endkernel;

MemoryManager &MemoryManager::get() {
	static MemoryManager instance;
	return instance;
}

void MemoryManager::init(multiboot_info_t* mbd, unsigned int magic) {
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		kn::panic("invalid magic number!");
	}
	size_t newaddr = reinterpret_cast<size_t>(mbd) + HIGHER_HALF_OFFSET;
	multiboot_info_t * map = reinterpret_cast<multiboot_info_t*>(newaddr);
	if(!(map->flags >> 6 & 0x1)) {
		kn::panic("Invalid memory map");
	}

	for(unsigned i = 0; i < map->mmap_length; 
		i += sizeof(multiboot_memory_map_t)) 
	{
		multiboot_memory_map_t* mmmt = 
			(multiboot_memory_map_t*) (map->mmap_addr + i + HIGHER_HALF_OFFSET);

		memsize += mmmt->len;

		// Tell the kernel that there is a bunch of available memory
		// in here
		if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
			// Below one megabyte lies the real memory region,
			// which seems to be a pita to reclaim, so skip it
			if((size_t)mmmt->addr < ONE_MEG) {
				continue;
			}
		} else {
			void * begin = reinterpret_cast<void*>(mmmt->addr + HIGHER_HALF_OFFSET);
			uint8_t* end = reinterpret_cast<uint8_t*>(
				reinterpret_cast<uintptr_t>(begin) + static_cast<size_t>(mmmt->len));
			used_regions[ureg_size++] = { 
				begin, 
				end-1
			};
		}
	}

	pages_bitmap = alloc_bitmap();
	real_memsize = memsize;
	for(size_t i = 0; i < ureg_size; i++) {
		real_memsize -= (
			reinterpret_cast<size_t>(used_regions[i].end) - 
			reinterpret_cast<size_t>(used_regions[i].begin));
	}
	log(INFO, "Total memory: %d B (%d KiB) (%d MiB)\n", memsize, memsize / ONE_KILO, memsize / ONE_MEG);
	log(INFO, "Available memory: %d B (%d KiB) (%d MiB)\n", real_memsize, real_memsize / ONE_KILO, real_memsize / ONE_MEG);
	alloc_pagevec();
	//PagingManager::get().heap_ready();
}

MemoryManager::bitmap_t * MemoryManager::alloc_bitmap() {
	// Get the address of the shitty heap I made
	// TODO: this explodes with more than 128 TiB - 4MiB of RAM,
	// allocate new regions if needed.
	bitmap_t* nextpage = reinterpret_cast<bitmap_t*>(INITIAL_MAPPING_NOHEAP + HIGHER_HALF_OFFSET);
	constexpr size_t divisor = PAGE_SIZE * BITS_PER_BYTE;
	constexpr size_t pages_divisor = divisor * PAGE_SIZE;
	bitmap_size = memsize / divisor;
	bitmap_n = memsize / sizeof(bitmap_t);
	bitmap_size_pages = (memsize + pages_divisor - 1) / pages_divisor;
	// Size of the initial mapping inside the bitmap
	const size_t orig_map_size_in_bitmap = INITIAL_MAPPING_WITHHEAP / divisor;

	// Fill already allocated regions with ones
	bitmap_t* addr = reinterpret_cast<bitmap_t*>(reinterpret_cast<uint8_t*>(nextpage) + orig_map_size_in_bitmap);
	for (bitmap_t *disp = nextpage; disp < addr; disp++) *disp = -1;

	// Mark these addresses as used
	used_regions[ureg_size++] = {
		reinterpret_cast<void*>(HIGHER_HALF_OFFSET),
		reinterpret_cast<void*>(INITIAL_MAPPING_WITHHEAP + HIGHER_HALF_OFFSET - 1)
	};

	return nextpage;
}

void *MemoryManager::alloc_pages(size_t pages, size_t map_flags) {
	// Total number of pages in the system
	size_t total_pages = memsize / PAGE_SIZE;

	// Iterating through the bitmap to find a range of free pages
	size_t consecutive_free = 0;
	size_t start_page = 0;

	constexpr size_t bitmap_size_bits = sizeof(bitmap_t) * BITS_PER_BYTE;

	for (size_t i = 0; i < total_pages; ++i) {
		size_t bit_index = i % bitmap_size_bits;
		size_t byte_index = i / bitmap_size_bits;
		// Check if the page is free in the bitmap
		if ((pages_bitmap[byte_index] & (1 << bit_index)) == 0) {
			if (consecutive_free == 0) {
				start_page = i;
			}
			consecutive_free++;
			// If we found the required number of pages, break
			if (consecutive_free == pages) {
				break;
			}
		} else {
			// Reset if a used page is found
			consecutive_free = 0;
		}
	}

	// If no suitable range was found, return NULL
	if (consecutive_free < pages) {
		return nullptr;
	}

	// Calculate the start address of the found pages
	void *start_addr = reinterpret_cast<void *>((start_page * PAGE_SIZE) + HIGHER_HALF_OFFSET);

	// Verify the address range does not overlap with used regions
	for (size_t i = 0; i < ureg_size; ++i) {
		for (size_t offset = 0; offset < pages; ++offset) {
			void *current_addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(start_addr) + offset * PAGE_SIZE);
			if (used_regions[i].contains(current_addr)) {
				// Address range invalid, abort allocation
				return nullptr;
			}
		}
	}
	PagingManager &pm = PagingManager::get();
	// Mark the pages as allocated in the bitmap
	for (size_t i = 0; i < pages; ++i) {
		size_t page_index = start_page + i;
		pages_bitmap[page_index / bitmap_size_bits] |= (1 << (page_index % bitmap_size_bits));
		size_t addr = reinterpret_cast<size_t>(start_addr) + i * PAGE_SIZE;
		void *current_addr = reinterpret_cast<void *>(addr - HIGHER_HALF_OFFSET);
		void *virtuaddr = reinterpret_cast<void*>(addr);

		pm.map_page(current_addr, virtuaddr, map_flags);
	}
	return start_addr;
}


void MemoryManager::free_pages(void *start, size_t pages) {
	uintptr_t address = reinterpret_cast<uintptr_t>(start);
	size_t page_number = address / PAGE_SIZE;
	// Check wether this overlaps with a used region
	// You should NEVER free a page here since the next
	// time you call alloc_pages(1) this will have
	// an aneuyrism
	for(size_t i = 0; i < ureg_size; i++) {
		auto & invaddress = used_regions[i];
		if(invaddress.contains(start)) return;
	}

	size_t byte_index = page_number / 8;
	size_t bit_index = page_number % 8;
	// Go around marking pages as deallocated
	for(size_t i = 0; i < pages; i++) {
		uint8_t val = ~(1 << bit_index);
		pages_bitmap[byte_index] &= val;

		bit_index++;
		if(bit_index >= 8) {
			bit_index = 0;
			byte_index++;
		}
	}
}

void MemoryManager::alloc_pagevec() {
	// The memory size in pages would be Memsize/4096
	// Out of all those pages, we only need 1 page out of 1024 to store a page table
	// so the result would be Memsize/4096/1024 pages for the page table vector
	// As each page table is itself 4096B long, we need Memsize/1024 bytes for this vector
	// So, for example: if we have 128 MiB of RAM, we have:
	// 32768 pages in total, from which we need 32 for this vector. As each table
	// is 4096 Bytes long, we need 4096*32 = 128 KiB

	// However, we actually need 8KiB less than that because we already have two page tables
	// (The "boot" ones)

	PagingManager &pm = PagingManager::get();

	pagevec_size = (memsize / 1024) - (2 * PAGE_SIZE);
	// Now we need to know if this vector would get out of the current mapped range.
	size_t bitmap_pages_bytes = bitmap_size_pages * PAGE_SIZE;
	bool outside_range = false;
	if(pagevec_size + bitmap_pages_bytes >= REGION_SIZE) {
		outside_range = true;
	}
	// Calculate the address
	size_t vector_addr = reinterpret_cast<size_t>(pages_bitmap) + bitmap_pages_bytes;
	pm.set_pagevec(reinterpret_cast<page_t*>(vector_addr));
	
	size_t initial_mapping = outside_range ? ((REGION_SIZE - bitmap_pages_bytes)/PAGE_SIZE) : (pagevec_size/PAGE_SIZE);
	for(size_t i = 0; i < initial_mapping; i++) {
		void * pt_addr = reinterpret_cast<void*>((i * PAGE_SIZE) + vector_addr);
		void * begin_with = reinterpret_cast<void*>(((i + 2) * REGION_SIZE) + HIGHER_HALF_OFFSET);
		pm.new_page_table(pt_addr, begin_with);
	}

	// If it's outside range, we have to map that first.
	if(outside_range) {
		size_t outside_bytes = (pagevec_size + bitmap_pages_bytes - (REGION_SIZE));
		size_t outside_pages = ((outside_bytes + PAGE_SIZE - 1) / PAGE_SIZE);
		
		for(size_t i = 0; i < outside_pages; i++) {
			size_t offset = (i*PAGE_SIZE) + INITIAL_MAPPING_WITHHEAP;
			pm.map_page(reinterpret_cast<void*>(offset), reinterpret_cast<void*>(offset + HIGHER_HALF_OFFSET), READ_WRITE);
		}
	}
}

used_region * MemoryManager::get_used_regions() {
	return used_regions;
}

size_t MemoryManager::get_used_regions_size() {
	return ureg_size;
}