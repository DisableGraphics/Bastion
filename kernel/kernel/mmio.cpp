#include <kernel/kernel/mmio.hpp>
#include <kernel/drivers/serial.hpp>
#include <stdio.h>

MMIO &MMIO::get() {
	static MMIO instance;
	return instance;
}

void MMIO::init(used_region *regions, size_t nregions) {
	// Last region is important because it's where all memory-mapped devices go
	used_region &last_region = regions[nregions-2];
	size_t npages = (reinterpret_cast<size_t>(last_region.end) 
		- reinterpret_cast<size_t>(last_region.begin) + PAGE_SIZE - 1) / PAGE_SIZE;
	size_t npage_tables = (npages + PAGE_SIZE - 1) / PAGE_SIZE;

	init_page_tables(last_region.begin, npage_tables);

	PagingManager &pm = PagingManager::get();
	for(size_t i = 0; i < npages; i++) {
		uintptr_t ptr = reinterpret_cast<uintptr_t>(last_region.begin) + PAGE_SIZE*i;
		void *raw_addr = reinterpret_cast<void*>(ptr);
		
		pm.map_page(raw_addr, raw_addr, CACHE_DISABLE | READ_WRITE);
	}
}

void MMIO::init_page_tables(void* origin, size_t n_page_tables) {
	page_tables = reinterpret_cast<page_t*>(MemoryManager::get().alloc_pages(n_page_tables));
	size_t addr = reinterpret_cast<size_t>(page_tables);
	PagingManager &pm = PagingManager::get();
	size_t originp = reinterpret_cast<size_t>(origin);
	for(size_t i = 0; i < n_page_tables; i++) {
		serial_printf("pointer: %p\n", originp);
		pm.new_page_table(reinterpret_cast<void*>(addr + i*PAGE_SIZE), reinterpret_cast<void*>(originp + REGION_SIZE*i));
	}
}