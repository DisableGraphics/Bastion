#pragma once
#include <stdint.h>

class PagingManager {
	public:
		static PagingManager &get();
		void init();
		void *get_physaddr(void *virtualaddr);
		void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
	private:
		[[gnu::aligned(4096)]] uint32_t page_directory[1024];
		[[gnu::aligned(4096)]] uint32_t page_table_1[1024];
		PagingManager(){}
};
extern uint32_t endkernel;