#include <stddef.h>
#include <kernel/mmanager.hpp>
#include <kernel/panic.hpp>
#include <kernel/const.hpp>
#include <kernel/page.hpp>
#include <stdio.h>
#include <kernel/pit.hpp>

#ifdef __i386
#include "../arch/i386/pagedef.h"
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

	if(!(mbd->flags >> 6 & 0x1)) {
		kn::panic("Invalid memory map");
	}

	for(unsigned i = 0; i < mbd->mmap_length; 
        i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = 
            (multiboot_memory_map_t*) (mbd->mmap_addr + i);

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
			void * begin = reinterpret_cast<void*>(mmmt->addr);
			void* end = reinterpret_cast<void*>(
    			static_cast<uintptr_t>(mmmt->addr) + static_cast<size_t>(mmmt->len));

			if(end >= begin) {
				invalid_addresses[indaddress_size++] = { 
					begin, 
					end
				};
			}
		}
    }
	printf("Available memory: %d B (%d KiB) (%d MiB)\n", memsize, memsize / ONE_KILO, memsize / ONE_MEG);
	pages_bitmap = alloc_bitmap();
	printf("Allocated for the pages bitmap beginning with: %p\n", pages_bitmap);
	for(size_t i = 0; i < indaddress_size; i++) {
		printf("%d to %d unavailable\n", invalid_addresses[i].begin, invalid_addresses[i].end);
	}
}

uint8_t * MemoryManager::alloc_bitmap() {
	// Get the address of the shitty heap I made
	uint8_t* nextpage = reinterpret_cast<uint8_t*>(INITIAL_MAPPING_NOHEAP);
	constexpr size_t divisor = PAGE_SIZE * BITS_PER_BYTE;
	constexpr size_t pages_divisor = divisor * PAGE_SIZE;
	bitmap_size = memsize / divisor;
	bitmap_size_pages = (memsize + pages_divisor - 1) / pages_divisor;

	/*for(size_t i = 0; i < bitmap_size_pages; i++) {
		void * addr = nextpage + i*PAGE_SIZE;
		PagingManager::get().map_page(addr, addr, READ_WRITE);
	}*/

	for(size_t i = 0; i < INITIAL_MAPPING_WITHHEAP; i++) {
		size_t bit_disp = i % (BITS_PER_BYTE*sizeof(bitmap_t));
		bitmap_t* disp = nextpage + (i / (BITS_PER_BYTE*sizeof(bitmap_t)));

		*disp |= (1 << bit_disp);
	}

	printf("Bitmap size %d bytes (%d pages)\n", bitmap_size, bitmap_size_pages);

	return nextpage;
}

void MemoryManager::add_to_pages_bitmap(multiboot_memory_map_t* mmmt) {

}