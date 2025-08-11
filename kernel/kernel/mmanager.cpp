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

extern uint32_t kernel_end;
extern uint32_t _kernel_start;

MemoryManager &MemoryManager::get() {
	static MemoryManager instance;
	return instance;
}

void MemoryManager::init(const multiboot_tag_mmap* mbd, const void* last_module_addr) {
	this->last_module_addr = last_module_addr;
	// entries size and total size (excluding first 16 bytes of the tag)
	uint32_t entry_size = mbd->entry_size;
	uint32_t entries_size = mbd->size - sizeof(multiboot_tag_mmap);
	auto mbd2 = const_cast<multiboot_tag_mmap*>(mbd);

	for (uint32_t offset = 0; offset < entries_size; offset += entry_size) {
		// Cast entries pointer + offset to an mmap entry pointer
		auto mmmt = reinterpret_cast<multiboot_mmap_entry*>(
			reinterpret_cast<uint8_t*>(mbd2->entries) + offset);

		memsize += mmmt->len;

		if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
			if (mmmt->addr < ONE_MEG) {
				continue;
			}
			// Handle available memory region here if you want
		} else {
			// Non-available memory region: mark as used region
			virtaddr_t begin = reinterpret_cast<virtaddr_t>((size_t)mmmt->addr + HIGHER_HALF_OFFSET);
			virtaddr_t end = reinterpret_cast<virtaddr_t>(
				begin + static_cast<size_t>(mmmt->len));
			used_regions[ureg_size++] = { (void*)begin, (void*)(end - 1) };
		}
	}

	pages_bitmap = alloc_bitmap();
	real_memsize = memsize;
	for(size_t i = 0; i < ureg_size; i++) {
		real_memsize -= (
			reinterpret_cast<size_t>(used_regions[i].end) - 
			reinterpret_cast<size_t>(used_regions[i].begin));
	}
	log(INFO, "Total memory: %d B (%d KiB) (%d MiB)", memsize, memsize / ONE_KILO, memsize / ONE_MEG);
	log(INFO, "Available memory: %d B (%d KiB) (%d MiB)", real_memsize, real_memsize / ONE_KILO, real_memsize / ONE_MEG);
	alloc_pagevec();
	//PagingManager::get().heap_ready();
}

MemoryManager::bitmap_t* MemoryManager::alloc_bitmap() {
	// Calculate kernel address range
	auto kstaaddr = reinterpret_cast<virtaddr_t>(&_kernel_start);
	auto kendaddr = reinterpret_cast<virtaddr_t>(&kernel_end > last_module_addr ? &kernel_end : last_module_addr);
	size_t kernel_size = kendaddr - kstaaddr;
	size_t kernel_pages = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE;
	size_t kernel_size_rounded_pages = kernel_pages * PAGE_SIZE;

	log(INFO, "Kernel size: %p bytes (%d pages)", kernel_size, kernel_pages);

	// Calculate number of pages in the system
	size_t num_pages = memsize / PAGE_SIZE;

	// Calculate bitmap size in bytes and pages (1 bit per page)
	bitmap_size_bytes = (num_pages + 7) / 8;
	bitmap_size_pages = (bitmap_size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

	// Place the bitmap right after the kernel in memory, aligned to PAGE_SIZE
	bitmap_t* bitmap_start = reinterpret_cast<bitmap_t*>(
		((kendaddr + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE
	);
	log(INFO, "Bitmap start address: %p", bitmap_start);

	// Clear bitmap memory
	memset(bitmap_start, 0, bitmap_size_pages * PAGE_SIZE);

	// Calculate how many bitmap_t entries correspond to kernel pages
	constexpr size_t bits_per_entry = sizeof(bitmap_t) * 8;
	size_t kernel_bitmap_entries = (kernel_pages + bits_per_entry - 1) / bits_per_entry;

	// Mark kernel pages as used (set bits to 1)
	for (size_t i = 0; i < kernel_bitmap_entries; i++) {
		bitmap_start[i] = (bitmap_t)(~0);
	}

	// Add kernel memory range to used_regions (inclusive)
	used_regions[ureg_size++] = {
		reinterpret_cast<void*>(HIGHER_HALF_OFFSET),
		reinterpret_cast<void*>(HIGHER_HALF_OFFSET + kernel_size_rounded_pages + bitmap_size_pages*PAGE_SIZE - 1)
	};

	log(INFO, "Marked used region: %p - %p", 
		(void*)HIGHER_HALF_OFFSET,
		(void*)(HIGHER_HALF_OFFSET + kernel_size_rounded_pages + bitmap_size_pages*PAGE_SIZE - 1));

	return bitmap_start;
}


void *MemoryManager::alloc_pages(size_t pages, size_t map_flags) {
	// Total number of pages in the system
	size_t total_pages = memsize / PAGE_SIZE;

	// Iterating through the bitmap to find a range of free pages
	size_t consecutive_free = 0;
	size_t start_page = 0;

	for (size_t i = 0; i < total_pages; ++i) {
		size_t bit_index = i % bits_per_bitmap_entry;
		size_t bitmap_index = i / bits_per_bitmap_entry;
		// Check if the page is free in the bitmap
		if ((pages_bitmap[bitmap_index] & (static_cast<bitmap_t>(1) << bit_index)) == 0) {
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
		log(INFO, "Not enough free pages");
		return nullptr;
	}

	auto& pm = PagingManager::get();

	// Calculate the start address of the found pages
	auto start_addr = reinterpret_cast<physaddr_t>((start_page * PAGE_SIZE));
	// Mark the pages as allocated in the bitmap
	for (size_t i = 0; i < pages; ++i) {
		size_t page_index = start_page + i;
		if((page_index / bits_per_bitmap_entry) >= bitmap_size_bytes) {
			kn::panic("Tried to access invalid position in the bitmap");
		}
		pages_bitmap[page_index / bits_per_bitmap_entry] |= (static_cast<bitmap_t>(1) << (page_index % bits_per_bitmap_entry));
		physaddr_t addr = start_addr + i * PAGE_SIZE;
		physaddr_t current_addr = reinterpret_cast<physaddr_t>(addr);
		virtaddr_t virtuaddr = reinterpret_cast<virtaddr_t>(addr + HIGHER_HALF_OFFSET);
		log(INFO, "(real) %p -> (virtual) %p", current_addr, virtuaddr)
		
		pm.map_page((void*)current_addr, (void*)virtuaddr, READ_WRITE);
	}
	log(INFO, "Allocated %d pages from %p", pages, start_addr);
	return reinterpret_cast<void*>(start_addr + HIGHER_HALF_OFFSET);
}


void MemoryManager::free_pages(void *start, size_t pages) {
	log(INFO, "Freeing from %p %d pages", start, pages);
	auto& pm = PagingManager::get();
	physaddr_t address = reinterpret_cast<physaddr_t>(start) - HIGHER_HALF_OFFSET;
	size_t page_number = address / PAGE_SIZE;
	size_t entry_index = page_number / bits_per_bitmap_entry;
	size_t bit_index = page_number % bits_per_bitmap_entry;
	// Go around marking pages as deallocated
	for(size_t i = 0; i < pages; i++, address += PAGE_SIZE) {
		bitmap_t mask = ~(static_cast<bitmap_t>(1) << bit_index);
		pages_bitmap[entry_index] &= mask;
		virtaddr_t virtual_addr = address + HIGHER_HALF_OFFSET;
		pm.unmap(reinterpret_cast<void*>(virtual_addr));
		bit_index++;
		if(bit_index >= bits_per_bitmap_entry) {
			bit_index = 0;
			entry_index++;
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

	// However, we actually need 8KiB less than that because we already have three page tables
	// (The "boot" ones)

	PagingManager &pm = PagingManager::get();

	constexpr int offset = 3;

	pagevec_size = (memsize / 1024) - (offset * PAGE_SIZE);
	// Now we need to know if this vector would get out of the current mapped range.
	size_t bitmap_pages_bytes = bitmap_size_pages * PAGE_SIZE;
	bool outside_range = false;
	if(pagevec_size + bitmap_pages_bytes >= REGION_SIZE) {
		outside_range = true;
	}
	// Calculate the address
	virtaddr_t vector_addr = reinterpret_cast<virtaddr_t>(pages_bitmap) + bitmap_pages_bytes;
	pm.set_pagevec(reinterpret_cast<page_t*>(vector_addr));
	
	size_t initial_mapping = outside_range ? ((REGION_SIZE - bitmap_pages_bytes)/PAGE_SIZE) : (pagevec_size/PAGE_SIZE);
	for(size_t i = 0; i < initial_mapping; i++) {
		virtaddr_t pt_addr = reinterpret_cast<virtaddr_t>((i * PAGE_SIZE) + vector_addr);
		virtaddr_t begin_with = reinterpret_cast<virtaddr_t>(((i + offset) * REGION_SIZE) + HIGHER_HALF_OFFSET);
		pm.new_page_table((void*)pt_addr, (void*)begin_with);
	}
	
	mark_as_used(reinterpret_cast<void*>(HIGHER_HALF_OFFSET), 
		reinterpret_cast<void*>(vector_addr + pagevec_size));
}

void MemoryManager::mark_as_used(void* start, void* end) {
	used_regions[ureg_size++] = {start, end};
	virtaddr_t start_uintptr = reinterpret_cast<virtaddr_t>(start);
	size_t start_page = (start_uintptr - HIGHER_HALF_OFFSET) / PAGE_SIZE;
	virtaddr_t end_uintptr = reinterpret_cast<virtaddr_t>(end);
	size_t end_page = (end_uintptr - HIGHER_HALF_OFFSET) / PAGE_SIZE;

	constexpr size_t bits_per_entry = sizeof(bitmap_t) * 8;

	size_t start_index = start_page / bits_per_entry;
	size_t start_bit = start_page % bits_per_entry;

	size_t end_index = end_page / bits_per_entry;
	size_t end_bit = end_page % bits_per_entry;

	if (start_index == end_index) {
		bitmap_t mask = ((bitmap_t)1 << (end_bit - start_bit + 1)) - 1;
		mask <<= start_bit;
		pages_bitmap[start_index] |= mask;
	} else {
		// First partial bitmap_t
		bitmap_t start_mask = (~(bitmap_t)0) << start_bit;
		pages_bitmap[start_index] |= start_mask;

		// Full bitmap_ts in between
		for (size_t i = start_index + 1; i < end_index; i++) {
			pages_bitmap[i] = ~(bitmap_t)0;  // all bits set
		}

		// Last partial bitmap_t
		bitmap_t end_mask = ((bitmap_t)1 << (end_bit + 1)) - 1;
		pages_bitmap[end_index] |= end_mask;
	}
	log(INFO, "Used region between %p and %p", start, end);
}

used_region * MemoryManager::get_used_regions() {
	return used_regions;
}

size_t MemoryManager::get_used_regions_size() {
	return ureg_size;
}

struct AllocationTrace {
	size_t n;
	void* address;
	size_t pages;
	const char* tag;
};

constexpr size_t MAX_ALLOCS = 1024;
AllocationTrace shadow_allocs[MAX_ALLOCS];
size_t shadow_alloc_count = 0;

void* MemoryManager::alloc_pages_debug(size_t pages, size_t map_flags) {
	void* ptr = alloc_pages(pages, map_flags);
	shadow_allocs[shadow_alloc_count++ % MAX_ALLOCS] = {
		shadow_alloc_count - 1, ptr, pages, "alloc_pages",
	};
	
	return ptr;
}

void MemoryManager::free_pages_debug(void *start, size_t pages) {
	free_pages(start, pages);
	if (start) {
		shadow_allocs[shadow_alloc_count++ % MAX_ALLOCS] = {
			shadow_alloc_count - 1, start, pages, "free_pages",
		};
	}
}

void MemoryManager::dump_recent_allocs() {
	log(ERROR, "=== Last %d tracked allocations ===", shadow_alloc_count);
	for (size_t i = 0; i < shadow_alloc_count; ++i) {
		auto& entry = shadow_allocs[i];
		log(ERROR, "[%d] |%s| %p (%d pages)", 
			i, entry.tag, entry.address, entry.pages);
	}
}
