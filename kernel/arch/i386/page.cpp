#include <stdint.h>
#include <kernel/page.hpp>
#include "kernel/inlineasm.h"
#include "kernel/serial.hpp"
#include "pagedef.h"
#include <error.h>

extern uint32_t boot_page_directory[1024];
extern uint32_t boot_page_table1[1024];

extern "C" void loadPageDirectory(uint32_t*);
extern "C" void enablePaging();

PagingManager &PagingManager::get() {
	static PagingManager instance;
	return instance;
}

void PagingManager::init() {
	page_directory = boot_page_directory;
	page_table_1 = boot_page_table1;
    
    //Map the second part
    for(unsigned i = 0; i < 1024; i++)
    {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes ;)
		page_table_2[i] = ((i * 0x1000) + INITIAL_MAPPING_NOHEAP) | (READ_WRITE | PRESENT);
    }
	size_t physical_addr = reinterpret_cast<size_t>(get_physaddr(page_table_2));
	page_directory[HIGHER_OFFSET_INDEX+1] = ((size_t) physical_addr) | (READ_WRITE | PRESENT);
	
	tlb_flush();
}

void * PagingManager::get_physaddr(void *virtualaddr) {
	unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return 0;
	uint32_t * page_table = (uint32_t*)((page_directory[pdindex] & ~0xFFF) + HIGHER_HALF_OFFSET);

    unsigned long *pt = ((unsigned long *)page_table) + (0x0400 * ptindex);
	if(!(page_table[ptindex] & PRESENT)) return 0;

   	return (void *)((page_table[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

void PagingManager::map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
    // Make sure that both addresses are page-aligned.
	if((unsigned long)physaddr % PAGE_SIZE != 0 || (unsigned long)virtualaddr % PAGE_SIZE != 0) {
		kerror("Addresses not page-aligned: %p, %p", physaddr, virtualaddr); return;
	}
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return; //TODO: Create new page

	unsigned long * page_table = (unsigned long*)(page_directory[pdindex] & ~0xFFF + HIGHER_HALF_OFFSET);

    unsigned long *pt = ((unsigned long *)page_table) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | PRESENT; // Present

	__asm__ __volatile__("invlpg (%0)" ::"r" (virtualaddr) : "memory");
}

void PagingManager::new_page_table(void *pt_addr, void *begin_with, bool use_heap) {
	if(use_heap)
		pagevec->push_back(reinterpret_cast<page_t*>(pt_addr));

	uint32_t * page_table = reinterpret_cast<uint32_t*>(pt_addr);
	unsigned long pdindex = (unsigned long)begin_with >> 22;

	for(size_t i = 0; i < 1024; i++) {
		page_table[i] = 0;
	}
	page_directory[pdindex] = (reinterpret_cast<uint32_t> (pt_addr) - HIGHER_HALF_OFFSET) | (READ_WRITE | PRESENT);

	tlb_flush();
}

void PagingManager::heap_ready() {
	pagevec = new Vector<page_t*>();
}

bool PagingManager::is_mapped(void *addr) {
	unsigned long pdindex = (unsigned long)addr >> 22;
    unsigned long ptindex = (unsigned long)addr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return false;
	uint32_t * page_table = (uint32_t*)((page_directory[pdindex] & ~0xFFF) + HIGHER_HALF_OFFSET);

    unsigned long *pt = ((unsigned long *)page_table) + (0x0400 * ptindex);
	if(!(page_table[ptindex] & PRESENT)) return false;
	return true;
}