#include <kernel/page.h>
#include "pagedef.h"
#include <error.h>
#include <stdio.h>

uint32_t boot_page_directory[1024];
uint32_t boot_page_table1[1024];

void page_initialize(void) {
    for(int i = 0; i < 1024; i++)
    {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them (1 if User-mode can access, 0 if not)
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
		//Note: no need to write the PRESENT flag as it is set to 0 by default
		// supervisor level (0), read/write (1), not present (0)
		boot_page_directory[i] = READ_WRITE;
    }
    // holds the physical address where we want to start mapping these pages to.
    // in this case, we want to map these pages to the very beginning of memory.
    
    //we will fill all 1024 entries in the table, mapping 4 megabytes
    for(unsigned int i = 0; i < 1024; i++)
    {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes ;)
		if(boot_page_directory[i] == 0)
        	boot_page_table1[i] = (i * 0x1000) | (READ_WRITE | PRESENT); // attributes: supervisor level, read/write, present.
    }
    boot_page_directory[0] = ((unsigned int)boot_page_table1) | (READ_WRITE | PRESENT);
}

void *get_physaddr(void *virtualaddr) {
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

	if(!(boot_page_directory[pdindex] & PRESENT)) return 0;
    // Here you need to check whether the PD entry is present.
	uint32_t * page_table = (uint32_t*)(boot_page_directory[pdindex] & ~0xFFF);

    unsigned long *pt = ((unsigned long *)page_table) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
	if(!(pt[ptindex] & PRESENT)) return 0;

    return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
    // Make sure that both addresses are page-aligned.
	if((unsigned long)physaddr % PAGE_SIZE != 0 || (unsigned long)virtualaddr % PAGE_SIZE != 0) {
		kerror("Addresses not page-aligned: %p, %p", physaddr, virtualaddr); return;
	}
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

	if(!(boot_page_directory[pdindex] & PRESENT)) return; //TODO: Create new page

	unsigned long * page_table = (unsigned long*)(boot_page_directory[pdindex] & ~0xFFF);

    unsigned long *pt = ((unsigned long *)page_table) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | PRESENT; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.

	__asm__ __volatile__("invlpg (%0)" ::"r" (virtualaddr) : "memory");
	printf("Mapped\n");
}

static const uint32_t startframe = 0x100000;
static const uint32_t npages = 0x1000;
static const uint32_t FREE = 0;
static const uint32_t USED = 1;
static const pageframe_t ERROR = {0xFFFFFFFF, 0};

pageframe_t kalloc_frame_int()
{
	uint32_t i;
    for(i = 0; boot_page_directory[i] != FREE; i++) {
		if(i == npages-1)
		{
			return(ERROR);
		}
    }
    boot_page_directory[i] = USED;
	pageframe_t ret = {startframe + (i*0x1000), 0};
    return ret;
}

void init_paging() {
	page_initialize();
}