#pragma once
#include <stdint.h>
#include <stddef.h>
#include <kernel/const.hpp>

constexpr size_t INITIAL_MAPPING = ONE_MEG * 4;

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
		[[gnu::aligned(4096)]] uint32_t page_directory[1024];
		[[gnu::aligned(4096)]] uint32_t page_table_1[1024];
		PagingManager(){}
};
extern uint32_t endkernel;