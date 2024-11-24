#include <kernel/page.hpp>
#include "pagedef.h"
#include <error.h>
#include <stdio.h>

extern "C" void loadPageDirectory(uint32_t*);
extern "C" void enablePaging();

PagingManager &PagingManager::get() {
	static PagingManager instance;
	return instance;
}

void PagingManager::init() {
    for(int i = 0; i < 1024; i++)
    {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them (1 if User-mode can access, 0 if not)
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
		//Note: no need to write the PRESENT flag as it is set to 0 by default
		// supervisor level (0), read/write (1), not present (0)
		page_directory[i] = READ_WRITE;
    }
    // holds the physical address where we want to start mapping these pages to.
    // in this case, we want to map these pages to the very beginning of memory.
    
    //we will fill all 1024 entries in the two tables, mapping 8 megabytes
    for(unsigned i = 0; i < 1024; i++)
    {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes ;)
		page_table_1[i] = (i * 0x1000) | (READ_WRITE | PRESENT); // attributes: supervisor level, read/write, present.
		page_table_2[i] = ((i * 0x1000) + INITIAL_MAPPING_NOHEAP) | (READ_WRITE | PRESENT);
    }
    page_directory[0] = ((unsigned int)page_table_1) | (READ_WRITE | PRESENT);
	page_directory[1] = ((unsigned int)page_table_2) | (READ_WRITE | PRESENT);

	loadPageDirectory(page_directory);
	enablePaging();
}

void * PagingManager::get_physaddr(void *virtualaddr) {
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return 0;
	uint32_t * page_table = (uint32_t*)(page_directory[pdindex] & ~0xFFF);

    unsigned long *pt = ((unsigned long *)page_table) + (0x400 * pdindex);
	if(!(pt[ptindex] & PRESENT)) return 0;

    return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

void PagingManager::map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
    // Make sure that both addresses are page-aligned.
	if((unsigned long)physaddr % PAGE_SIZE != 0 || (unsigned long)virtualaddr % PAGE_SIZE != 0) {
		kerror("Addresses not page-aligned: %p, %p", physaddr, virtualaddr); return;
	}
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return; //TODO: Create new page

	unsigned long * page_table = (unsigned long*)(page_directory[pdindex] & ~0xFFF);

    unsigned long *pt = ((unsigned long *)page_table) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | PRESENT; // Present

	__asm__ __volatile__("invlpg (%0)" ::"r" (virtualaddr) : "memory");
}