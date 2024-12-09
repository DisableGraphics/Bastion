#include <kernel/memory/mmanager.hpp>

class MMIO {
	public:
		static MMIO& get();
		void init(used_region* regions, size_t nregions);
	private:
		MMIO(){}
};