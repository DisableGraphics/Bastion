#include <kernel/memory/mmanager.hpp>

class MMIO {
	public:
		static MMIO& get();
		void init(used_region* regions, size_t nregions);
	private:
		void init_page_tables(void* origin, size_t n_page_tables);
		MMIO(){}
		page_t *page_tables;
};