#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/page.h>

extern "C" void loadPageDirectory(uint32_t*);
extern "C" void enablePaging();

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void init_paging() {
	loadPageDirectory(page_directory);
	page_initialize();
	enablePaging();
}

extern "C" void kernel_main(void) {
	terminal_initialize();
	printf("Hello, kernel World!\n");
	init_paging();
}
