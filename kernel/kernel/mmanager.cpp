#include <kernel/mmanager.hpp>
#include <kernel/panic.hpp>

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

        printf("Start Addr: %p | Length: %p | Size: %p | Type: %d | ",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);

        if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
			printf("Available\n");
            /* 
             * Do something with this memory block!
             * BE WARNED that some of memory shown as availiable is actually 
             * actively being used by the kernel! You'll need to take that
             * into account before writing to memory!
             */
        } else {
			printf("Not available\n");
		}
    }
}