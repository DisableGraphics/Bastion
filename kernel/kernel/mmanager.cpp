#include <stddef.h>
#include <kernel/mmanager.hpp>
#include <kernel/panic.hpp>
#include <kernel/const.hpp>
#include <stdio.h>

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
			add_to_pages_bitmap(mmmt);
        }
    }
}

void MemoryManager::add_to_pages_bitmap(multiboot_memory_map_t* mmmt) {
	
}