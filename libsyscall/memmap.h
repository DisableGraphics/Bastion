#ifndef __MMAP_BASTION_H
#define __MMAP_BASTION_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
/*
	Struct definition for the memory map given at boot time to the first task
*/

// Address from which all the memory starts being offset mapped
#define OFFSET_MAP 0x40000000
// Virtual address of the memory map (1 MiB before all the memory)
#define MMAP_VIRTADDR (OFFSET_MAP - (1 << 20))

struct mmap_entry {
	size_t addrflags; // Contains address (aligned to at least 4096) and the region flags in bits 0-11
	size_t npages;
};
struct mmap {
	size_t size;
	struct mmap_entry entries[];
};
typedef struct mmap mmap_t;
typedef struct mmap_entry mmap_entry_t;

inline size_t get_address(size_t addrflags) {
	return addrflags & ~0xFFF;
}

inline size_t get_flags(size_t addrflags) {
	return addrflags & 0xFFF;
}

// Flags
#define MEM_DEVICE (1 << 1)
#define MEM_FB (1 << 2)
#define MEM_USABLE (1 << 3)

#ifdef __cplusplus
}
#endif
#endif