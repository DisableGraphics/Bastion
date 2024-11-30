#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/const.hpp>
#include <kernel/datastr/vector.hpp>

#ifdef __i386
#include "../arch/i386/pagedef.h"
#endif

constexpr size_t INITIAL_MAPPING_NOHEAP = ONE_MEG * 4;
constexpr size_t REGION_SIZE = ONE_MEG * 4;
constexpr size_t INITIAL_MAPPING_WITHHEAP = INITIAL_MAPPING_NOHEAP * 2;
constexpr size_t HIGHER_HALF_OFFSET = 0xC0000000;
constexpr size_t HIGHER_OFFSET_INDEX = HIGHER_HALF_OFFSET >> 22;

/**
	\brief Pages manager
	Manages physical and virtual pages.
	Implemented as a singleton.

	Has a very close relationship with the Memory Manager.
	The Memory Manager keeps track of free/used pages.
	while the Paging Manager keeps tabs on page tables themselves.
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
		/**
			\brief Setup a new page table.
			\param pt_addr Address of the new page table
			\param use_heap Use a vector to keep track of the page.

			Note: use_heap should be true for all page tables, except for
			one in particular that it's used to piggyback memory allocations
			after the kernel space. That page table is allocated by the Memory Manager.
		 */
		void new_page_table(void* pt_addr, bool use_heap = true);
		/**
			\brief Whether an address is mapped to a physical page or not
			\param addr Address to check
			\return true if address is mapped, false if it's not.
		 */
		bool is_mapped(void* addr);
		/**
			\brief Signal that the heap is ready for future page table allocation
			requests. Initialises the pagevec vector since it requires heap space.
		 */
		void heap_ready();
	private:
		typedef uint32_t page_t;
		// Kernel page directory
		page_t *page_directory;
		// Kernel page table 1
		page_t *page_table_1;
		// Kernel page table 2
		// This table is the "heap" of the kernel since gcc is putting
		// cursed shit after the end of the kernel and I can't know where the
		// cursed shit finishes. So the strategy here is a "fuckit" one.
		// Is this good? maybe not. Will it give problems? Maybe yes.
		// Is this good enough for now? abso-fucking-lutely
		// In all seriousness tho, I need space for the pages bitmap
		[[gnu::aligned(PAGE_SIZE)]] page_t page_table_2[1024];

		// Vector of pointers to pages to keep them in check
		Vector<page_t*>* pagevec = nullptr;
		PagingManager(){}
};
extern uint32_t endkernel;