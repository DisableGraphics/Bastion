#include <stdint.h>
#include <kernel/fs/partmanager.hpp>
#include "../arch/i386/defs/fs/fat32/bs.hpp"

class FAT32 {
	public:
		FAT32(PartitionManager &partmanager, size_t partid);
		~FAT32();

		bool read(const char* filename, unsigned offset, unsigned nbytes, char* buffer);
		bool write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer);
	private:
		uint32_t get_sector_of_cluster(uint32_t cluster);
		uint32_t next_cluster(uint32_t active_cluster);
		bool load_cluster(uint32_t cluster, uint8_t* buffer);
		uint32_t cluster_for_directory(const char* directory);
		uint32_t cluster_for_filename(const char* filename, unsigned offset);
		size_t partid;
		char partname[12];
		uint8_t* fat_boot_buffer;
		fat_BS_t* fat_boot;
		uint32_t total_sectors;
		uint32_t fat_size;
		uint32_t first_data_sector, first_fat_sector;
		uint32_t n_data_sectors;
		uint32_t total_clusters;
		uint32_t root_cluster;
		uint32_t sector_size;
		PartitionManager &partmanager;
};