#ifndef PAGE_H
#define PAGE_H

#define PAGE_SIZE 4096

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint32_t page_directory[1024] __attribute__((aligned(4096)));
extern uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void page_initialize(void);
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void *get_physaddr(void *virtualaddr);

#ifdef __cplusplus
}
#endif

#endif