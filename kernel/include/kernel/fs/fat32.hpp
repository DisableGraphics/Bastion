#include <kernel/fs/partmanager.hpp>

class FAT32 {
	public:
		FAT32(PartitionManager &partmanager, size_t partid);
		
	private:
		size_t partid;
};