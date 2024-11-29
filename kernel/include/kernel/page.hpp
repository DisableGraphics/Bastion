#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/const.hpp>

#ifdef __i386
#include "../arch/i386/pagedef.h"
#endif

constexpr size_t INITIAL_MAPPING_NOHEAP = ONE_MEG * 4;
constexpr size_t INITIAL_MAPPING_WITHHEAP = INITIAL_MAPPING_NOHEAP * 2;
constexpr size_t HIGHER_HALF_OFFSET = 0xC0000000;
constexpr size_t HIGHER_OFFSET_INDEX = HIGHER_HALF_OFFSET >> 22;

/**
	\brief Pages manager
	Manages physical and virtual pages.
	Implemented as a singleton
 */
class PagingManager {
	public:
		/**
			\brief Get singleton instance
		 */
		static PagingManager &get();
		/**
			\brief Initialise the paging manager
		 */
		void init();
		/**
			\brief Get a physical address from a virtual address
			\param virtualaddr Virtual address to get
		 */
		void *get_physaddr(void *virtualaddr);
		/**
			\brief map a physical page frame in memory to a virtual address
			\param physaddr Physical address of page frame
			\param virtualaddr Virtual address of the page
			\param flags Flags to add to the mapping
		 */
		void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
	private:
		// Kernel page directory
		uint32_t *page_directory;
		// Kernel page table 1
		uint32_t *page_table_1;
		// Kernel page table 2
		// This table is the "heap" of the kernel since gcc is putting
		// cursed shit after the end of the kernel and I can't know where the
		// cursed shit finishes. So the strategy here is a "fuckit" one.
		// Is this good? maybe not. Will it give problems? Maybe yes.
		// Is this good enough for now? abso-fucking-lutely
		// In all seriousness tho, I need space for the pages bitmap
		[[gnu::aligned(PAGE_SIZE)]] uint32_t page_table_2[1024];
		PagingManager(){}
};
extern uint32_t endkernel;