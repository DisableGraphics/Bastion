#include <kernel/kernel/mmio.hpp>

MMIO &MMIO::get() {
	static MMIO instance;
	return instance;
}

void MMIO::init(used_region *regions, size_t nregions) {
	// Last region is important because it's where all memory-mapped devices go
	used_region &last_region = regions[nregions-1];
	size_t npages = (reinterpret_cast<size_t>(last_region.end) 
		- reinterpret_cast<size_t>(last_region.begin)) / PAGE_SIZE;
	PagingManager &pm = PagingManager::get();
	for(size_t i = 0; i < npages; i++) {
		uintptr_t ptr = reinterpret_cast<uintptr_t>(last_region.begin) + PAGE_SIZE*i;
		void *raw_addr = reinterpret_cast<void*>(ptr);
		pm.map_page(raw_addr, raw_addr, CACHE_DISABLE | READ_WRITE);
	}
}