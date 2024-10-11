#include <stdio.h>
#include <kernel/page.h>
#include <assert.h>
#include <stddef.h>

void test_paging() {
	printf("Testing paging... ");
	int * physaddr = (int *)0x1000;
	int * virtualaddr = (int *)0x2000;
	map_page(physaddr, virtualaddr, 0x3);
	*virtualaddr = 12345678;
	assert(*(int*)get_physaddr(virtualaddr) == 12345678);
	assert((size_t)get_physaddr(virtualaddr) == (size_t)physaddr);
	printf("Ok\n");
}