#include <stddef.h>
#include <kernel/mmanager.hpp>
#include <kernel/panic.hpp>
#include <kernel/const.hpp>
#include <kernel/page.hpp>
#include <stdio.h>
#include <kernel/pit.hpp>

#ifdef __i386
#include "../arch/i386/pagedef.h"
#include "../arch/i386/defs/max_addr_size.hpp"
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
			uint8_t* end = reinterpret_cast<uint8_t*>(
    			static_cast<uintptr_t>(mmmt->addr) + static_cast<size_t>(mmmt->len));
			invalid_addresses[indaddress_size++] = { 
				begin, 
				end-1
			};
		}
    }

	pages_bitmap = alloc_bitmap();
	real_memsize = memsize;
	for(size_t i = 0; i < indaddress_size; i++) {
		real_memsize -= (
			reinterpret_cast<size_t>(invalid_addresses[i].end) - 
			reinterpret_cast<size_t>(invalid_addresses[i].begin));
	}
	printf("Total memory: %d B (%d KiB) (%d MiB)\n", memsize, memsize / ONE_KILO, memsize / ONE_MEG);
	printf("Available memory: %d B (%d KiB) (%d MiB)\n", real_memsize, real_memsize / ONE_KILO, real_memsize / ONE_MEG);
}

uint8_t * MemoryManager::alloc_bitmap() {
	// Get the address of the shitty heap I made
	//TODO: this explodes with more than 128 TiB of RAM
	// allocate new regions if needed.
	uint8_t* nextpage = reinterpret_cast<uint8_t*>(INITIAL_MAPPING_NOHEAP);
	constexpr size_t divisor = PAGE_SIZE * BITS_PER_BYTE;
	constexpr size_t pages_divisor = divisor * PAGE_SIZE;
	bitmap_size = memsize / divisor;
	bitmap_size_pages = (memsize + pages_divisor - 1) / pages_divisor;

	for(size_t i = 0; i < INITIAL_MAPPING_WITHHEAP; i++) {
		size_t bit_disp = i % (BITS_PER_BYTE*sizeof(bitmap_t));
		bitmap_t* disp = nextpage + (i / (BITS_PER_BYTE*sizeof(bitmap_t)));

		*disp |= (1 << bit_disp);
	}
	// Mark these addresses as used
	invalid_addresses[indaddress_size++] = {
		NULL,
		reinterpret_cast<void*>(INITIAL_MAPPING_WITHHEAP)
	};

	return nextpage;
}

void * MemoryManager::alloc_pages(size_t pages) {
	if (pages == 0 || !pages_bitmap) return nullptr;

    size_t consecutive_free = 0;
    size_t start_page = 0;

    for (size_t i = 0; i < bitmap_size * 8; ++i) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        if (!(pages_bitmap[byte_index] & (1 << bit_index))) {
            if (consecutive_free == 0) start_page = i;
            ++consecutive_free;
            if (consecutive_free == pages) {
                uintptr_t start_addr = reinterpret_cast<uintptr_t>(start_page * PAGE_SIZE);
                uintptr_t end_addr = start_addr + pages * PAGE_SIZE;

                bool overlaps = false;
                for (size_t j = 0; j < indaddress_size; ++j) {
                    uintptr_t invalid_begin = reinterpret_cast<uintptr_t>(invalid_addresses[j].begin);
                    uintptr_t invalid_end = reinterpret_cast<uintptr_t>(invalid_addresses[j].end);

                    if (!(end_addr <= invalid_begin || start_addr >= invalid_end)) {
                        overlaps = true;
                        break;
                    }
                }

                if (overlaps) {
                    consecutive_free = 0; // Reset the search
                    continue;
                }

                // Mark the pages as allocated
                for (size_t j = 0; j < pages; ++j) {
                    size_t alloc_byte = (start_page + j) / 8;
                    size_t alloc_bit = (start_page + j) % 8;
                    pages_bitmap[alloc_byte] |= (1 << alloc_bit); // Mark bit as allocated
                }

                // Calculate the starting physical address and return it
                uintptr_t address = start_page * 4096; // Start address
                return reinterpret_cast<void *>(address);
            }
        } else {
            // Reset counter if a used page is encountered
            consecutive_free = 0;
        }
    }
    return nullptr;
}

void MemoryManager::free_pages(void *start, size_t pages) {
	uintptr_t address = reinterpret_cast<uintptr_t>(start);
    size_t page_number = address / PAGE_SIZE;

	for(size_t i = 0; i < indaddress_size; i++) {
		auto & invaddress = invalid_addresses[i];
		if(invaddress.begin >= start && invaddress.end <= start) return;
	}

	size_t byte_index = page_number / 8;
	size_t bit_index = page_number % 8;

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