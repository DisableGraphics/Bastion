#include <kernel/fs/partmanager.hpp>

class FAT32 {
	public:
		FAT32(PartitionManager &partmanager, size_t partid);

	private:
		size_t partid;
		char partname[12];
		uint32_t total_sectors;
		uint32_t fat_size;
};