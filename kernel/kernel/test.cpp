#include <stdio.h>
#include <kernel/page.hpp>
#include <assert.h>
#include <stddef.h>

void test_paging() {
	PagingManager &page = PagingManager::get();
	printf("Testing paging... ");
	int * physaddr = (int *)0x1000;
	int * virtualaddr = (int *)0x2000;
	page.map_page(physaddr, virtualaddr, 0x3);
	*virtualaddr = 12345678;
	assert(*(int*)page.get_physaddr(virtualaddr) == 12345678);
	assert((size_t)page.get_physaddr(virtualaddr) == (size_t)physaddr);
	printf("Ok\n");
}