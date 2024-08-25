#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/page.h>

extern "C" void loadPageDirectory(uint32_t*);
extern "C" void enablePaging();

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void init_gdt() {
	// First disable interrupts
	__asm__ __volatile("cli");
}

void init_paging() {
	loadPageDirectory(page_directory);
	page_initialize();
	enablePaging();
}

void test_paging() {
	printf("Testing paging\n");
	void * physaddr = (void *)0x1000;
	void * virtualaddr = (void *)0x2000;
	map_page(physaddr, virtualaddr, 0x3);
	printf("Mapped %p to %p\n", physaddr, virtualaddr);
	printf("Test: %p (Sould be %p)\n", get_physaddr(virtualaddr), physaddr);
	printf("PD_PART: %d, PT_PART: %d, DISP: %d\n", (unsigned long)virtualaddr >> 22, (unsigned long)virtualaddr >> 12 & 0x03FF, (unsigned long)virtualaddr & 0xFFF);
	*(int*)virtualaddr = 12345678;
	printf("Value at %p: %d\n", virtualaddr, *(int*)get_physaddr(virtualaddr));
}

extern "C" void kernel_main(void) {
	terminal_initialize();
	printf("Initializing paging...\t");
	init_paging();
	printf("OK\n");
	map_page((void*)0xB8000, (void*)0xB8000, 0x7);
	test_paging();
	for(int i = 0; i < 2; i++) {
		printf("page_directory[%d] = %p\n", i, page_directory[i] & ~0xFFF);
	}
	for(int i = 0; i < 2; i++) {
		printf("first_page_table[%d] = %p\n", i, first_page_table[i] & ~0xFFF);
	}
}
