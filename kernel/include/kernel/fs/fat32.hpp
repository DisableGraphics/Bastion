#include <kernel/fs/partmanager.hpp>
#include <kernel/sync/semaphore.hpp>
#include "../arch/i386/defs/fs/fat32/bs.hpp"

class FAT32 {
	public:
		FAT32(PartitionManager &partmanager, size_t partid);
		~FAT32();
	private:
		uint32_t get_sector_of_cluster(uint32_t cluster);
		size_t partid;
		char partname[12];
		uint8_t fat_boot_buffer[512];
		fat_BS_t* fat_boot;
		uint32_t total_sectors;
		uint32_t fat_size;
		uint32_t first_data_sector, first_fat_sector;
		uint32_t n_data_sectors;
		uint32_t total_clusters;
		uint32_t root_cluster;
};