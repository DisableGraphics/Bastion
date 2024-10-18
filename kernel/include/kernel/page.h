#pragma once
#include <stdint.h>

class PagingManager {
	public:
		void init();
		void *get_physaddr(void *virtualaddr);
		void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
	private:
		uint32_t page_directory[1024] __attribute__((aligned(4096)));
		uint32_t page_table_1[1024] __attribute__((aligned(4096)));
};

extern PagingManager page;

#ifdef __cplusplus
extern "C" {
#endif
extern uint32_t endkernel;

struct pageframe_t {
    uint32_t frame;
    struct pageframe_t *next;
};

typedef struct pageframe_t pageframe_t;

pageframe_t kalloc_frame_int();
pageframe_t kalloc_frame();
void kfree_frame(pageframe_t a);

#ifdef __cplusplus
}
#endif