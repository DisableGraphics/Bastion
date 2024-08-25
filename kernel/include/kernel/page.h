#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint32_t page_directory[1024] __attribute__((aligned(4096)));
extern uint32_t first_page_table[1024] __attribute__((aligned(4096)));

extern uint32_t endkernel;

void page_initialize(void);
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void *get_physaddr(void *virtualaddr);

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