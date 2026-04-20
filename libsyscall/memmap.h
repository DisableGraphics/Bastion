#ifndef __MMAP_BASTION_H
#define __MMAP_BASTION_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
/*
	Struct definition for the memory map given at boot time to the first task
*/

/**
The memory map has the following structure:
-----------------------------------------
|										|
|		   Number of entries (8B)		|
|										|
-----------------------------------------
|										|
|				Entries			 		|
| -------------------------------------	|
| |			  Address+flags (8B)	  |	|
| -------------------------------------	|
| |			   Nº pages (8B)		  |	|
| -------------------------------------	|
| |	   Optional (Depends on entry)	  |	|
| -------------------------------------	|
-----------------------------------------
*/

// Address from which all the memory starts being offset mapped
#define OFFSET_MAP 0x40000000
// Virtual address of the memory map (1 MiB before all the memory)
#define MMAP_VIRTADDR (OFFSET_MAP - (1 << 20))

struct mmap_basic_entry {
	size_t addrflags; // Contains address (aligned to at least 4096) and the region flags in bits 0-11
	size_t npages;
};
struct mmap {
	size_t size;
	size_t entries[];
};
typedef struct mmap mmap_t;
typedef struct mmap_basic_entry mmap_basic_entry_t;

typedef struct mmap_fb_entry {
	struct mmap_basic_entry defs;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t bpp;
	uint32_t red_mask_size;
	uint32_t green_mask_size;
	uint32_t blue_mask_size;
	uint8_t red_mask_disp;
	uint8_t green_mask_disp;
	uint8_t blue_mask_disp;
} mmap_fb_entry_t;

/**
	\brief Returns the address on the address+flags field 
*/
inline size_t get_address(size_t addrflags) {
	return addrflags & ~0xFFF;
}

/**
	\brief Returns the flags on the address+flags field
*/
inline size_t get_flags(size_t addrflags) {
	return addrflags & 0xFFF;
}

/// Flags
//// Reserved memory
#define MEM_DEVICE (1 << 1)
//// Framebuffer
#define MEM_FB (1 << 2)
//// Actual usable memory
#define MEM_USABLE (1 << 3)

/**
	\brief Gets next entry. If current entry is set to NULL, it returns the first one.
	\warning If current is the last entry it will return uninitialized memory.
	\param memmap The memory map
	\param current Current entry or NULL to get first entry
*/
inline mmap_basic_entry_t* get_next_entry(mmap_t* memmap, mmap_basic_entry_t* current) {
	if(current == NULL) return (mmap_basic_entry_t*)&memmap->entries[0];
	mmap_basic_entry_t* ret = NULL;
	switch(get_flags(current->addrflags)) {
		case MEM_FB:
			ret = current + (sizeof(mmap_fb_entry_t)/sizeof(size_t));
		default:
			ret = current + (sizeof(mmap_basic_entry_t)/sizeof(size_t));
	}
	return ret;
}

/**
	\brief Gets nth entry.
	\warning Doesn't do bounds checking.
	\param memmap The memory map.
	\param n nth entry. Starts from zero like an array.
*/
inline mmap_basic_entry_t* get_nth_entry(mmap_t* memmap, size_t n) {
	mmap_basic_entry_t* ret = get_next_entry(memmap, NULL);
	for(size_t i = 0; i < n; i++) {
		ret = get_next_entry(memmap, ret);
	}
	return ret;
}

#ifdef __cplusplus
}
#endif
#endif