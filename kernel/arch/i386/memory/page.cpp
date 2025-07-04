#include <cpuid.h>
#include <stdint.h>
#include <kernel/memory/page.hpp>
#include <kernel/assembly/inlineasm.h>
#include "../defs/page/pagedef.h"
#include <error.h>

extern uint32_t boot_page_directory[1024];
extern uint32_t boot_page_table1[1024];

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

	if(!(page_table[ptindex] & PRESENT)) return 0;

   	return (void *)((page_table[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

void PagingManager::map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
	// Make sure that both addresses are page-aligned.
	if((size_t)physaddr % PAGE_SIZE != 0 || (size_t)virtualaddr % PAGE_SIZE != 0) {
		kerror("Addresses not page-aligned: %p, %p", physaddr, virtualaddr); return;
	}
	size_t pdindex = (size_t)virtualaddr >> 22;
	size_t ptindex = (size_t)virtualaddr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return;

	uint32_t * page_table = reinterpret_cast<uint32_t*>((page_directory[pdindex] & ~0xFFF) + HIGHER_HALF_OFFSET);

	//uint32_t *pt = (page_table) + (0x400 * pdindex);
	// Here you need to check whether the PT entry is present.
	// When it is, then there is already a mapping present. What do you do now?

	page_table[ptindex] = (reinterpret_cast<size_t>(physaddr)) | (flags & 0xFFF) | PRESENT; // Present

	asm volatile("invlpg (%0)" ::"r"(virtualaddr) : "memory");
}

void PagingManager::unmap(void* virtaddr) {
	if((size_t)virtaddr % PAGE_SIZE != 0) {
		kerror("Addresses not page-aligned: %p", virtaddr); return;
	}
	size_t pdindex = (size_t)virtaddr >> 22;
	size_t ptindex = (size_t)virtaddr >> 12 & 0x03FF;
	if(!(page_directory[pdindex] & PRESENT)) return;

	uint32_t * page_table = reinterpret_cast<uint32_t*>((page_directory[pdindex] & ~0xFFF) + HIGHER_HALF_OFFSET);

	page_table[ptindex] = 0;
	asm volatile("invlpg (%0)" ::"r"(virtaddr) : "memory");
}

void PagingManager::set_global_options(void* virtualaddr, unsigned int flags) {
	if((size_t)virtualaddr % PAGE_SIZE != 0) {
		kerror("Addresses not page-aligned: %p, %p", virtualaddr, virtualaddr); return;
	}
	size_t pdindex = (size_t)virtualaddr >> 22;

	if(!(page_directory[pdindex] & PRESENT)) return;
	page_directory[pdindex] |= (flags & 0xFFF);
}

void PagingManager::new_page_table(void *pt_addr, void *begin_with) {
	uint32_t * page_table = reinterpret_cast<uint32_t*>(pt_addr);
	unsigned long pdindex = (unsigned long)begin_with >> 22;

	memset(page_table, 0, PAGE_SIZE);
	page_directory[pdindex] = (reinterpret_cast<uint32_t> (pt_addr) - HIGHER_HALF_OFFSET) | (READ_WRITE | PRESENT);

	tlb_flush();
}

bool PagingManager::is_mapped(void *addr) {
	unsigned long pdindex = (unsigned long)addr >> 22;
	unsigned long ptindex = (unsigned long)addr >> 12 & 0x03FF;

	if(!(page_directory[pdindex] & PRESENT)) return false;
	uint32_t * page_table = (uint32_t*)((page_directory[pdindex] & ~0xFFF) + HIGHER_HALF_OFFSET);

	if(!(page_table[ptindex] & PRESENT)) return false;
	return true;
}

void PagingManager::set_pagevec(page_t * pagevec) {
	pt_vector = pagevec;
}

bool PagingManager::page_table_exists(void *addr) {
	unsigned long pdindex = (unsigned long)addr >> 22;
	return page_directory[pdindex] & PRESENT;
}

#define PAT_TYPE_WC  0x01  // Write-Combining
#define PAT_TYPE_UC  0x00  // Uncached

bool check_pat_support() {
    uint32_t eax, edx, unused;
    __cpuid(1, eax, unused, unused, edx);
    return (edx & (1 << 16)); // CPUID.01h:EDX.PAT[bit 16]
}

bool PagingManager::enable_pat_if_it_exists() {
	if(check_pat_support()) {
		uint64_t pat_msr = 0;
    
		// Set up custom PAT entries
		pat_msr |= (0b001 << 3);
		
		// Write to IA32_PAT MSR (0x277)
		wrmsr(0x277, pat_msr);
		return true;
	}
	return false;
}

void *PagingManager::get_page_directory() {
	return boot_page_directory;
}