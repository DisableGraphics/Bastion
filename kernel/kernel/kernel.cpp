#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/page.h>
#include <kernel/tty.hpp>
#include <assert.h>
#include <kernel/gdt.h>

uint8_t gdt[48];

void test_paging() {
	printf("Testing paging... ");
	uint32_t shitty_page_table[1024] __attribute__((aligned(4096)));
	int * physaddr = (int *)0x1000;
	int * virtualaddr = (int *)0x2000;
	map_page(physaddr, virtualaddr, 0x3);
	*virtualaddr = 12345678;
	assert(*(int*)get_physaddr(virtualaddr) == 12345678);
	assert((size_t)get_physaddr(virtualaddr) == (size_t)physaddr);
	printf("Ok\n");
}

extern "C" void kernel_main(void) {
	init_gdt();
	
	terminal_initialize();
	printf("Initializing booting sequence\n");
	init_paging();
	//test_paging();

	/*for(int i=0; i<1024;i++) {
		uint32_t addr = boot_page_directory[i] & ~0xFFF;
		if(addr)
			printf("%d %p \n", i, addr);
	}*/
	for(int i=0; i<1024;i++) {
		uint32_t addr = boot_page_table1[i] & ~0xFFF;
		if(addr)
			printf("%d %p \n", i, addr);
	}
	printf("Finished booting. Giving control to the init process.");
}
