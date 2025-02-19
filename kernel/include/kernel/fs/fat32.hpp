#include <stdint.h>
#include <kernel/fs/partmanager.hpp>
#include "../arch/i386/defs/fs/fat32/bs.hpp"
#include "../arch/i386/defs/fs/fat32/entryflags.hpp"
#include "const.hpp"

class FAT32 {
	public:
		FAT32(PartitionManager &partmanager, size_t partid);
		~FAT32();

		off_t read(const char* filename, unsigned offset, unsigned nbytes, char* buffer);
		off_t write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer);
		off_t truncate(const char* filename, unsigned nbytes);
		int stat(const char* filename, stat* buf);
		
	private:
		uint32_t get_sector_of_cluster(uint32_t cluster);
		uint32_t next_cluster(uint32_t active_cluster);
		bool set_next_cluster(uint32_t cluster, uint32_t next_cluster);
		bool load_cluster(uint32_t cluster, uint8_t* buffer);
		bool save_cluster(uint32_t cluster, uint8_t* buffer);
		uint32_t first_cluster_for_directory(const char* directory);
		uint32_t cluster_for_filename(const char* filename, unsigned offset);
		bool filecmp(const char* basename, const char* entrydata, bool lfn);
		uint32_t match_cluster(uint8_t* cluster, const char* basename, FAT_FLAGS flags, struct stat* statbuf = nullptr, int* nentry = nullptr);
		void direntrystat(uint8_t* direntry, struct stat* statbuf, uint32_t cluster);
		bool update_fsinfo(int diffclusters);
		
		bool file_setproperty(const char* filename, const struct stat* properties);
		bool setstat(uint32_t dircluster, int nentry, const struct stat* properties);
		bool alloc_clusters(uint32_t prevcluster, uint32_t nclusters);
		uint32_t search_free_cluster(uint32_t searchfrom);

		size_t partid;
		char partname[12];
		uint8_t* fat_boot_buffer;
		fat_BS_t* fat_boot;
		uint32_t fsinfo_sector;
		uint32_t total_sectors;
		uint32_t fat_size;
		uint32_t first_data_sector, first_fat_sector;
		uint32_t n_data_sectors;
		uint32_t total_clusters;
		uint32_t root_cluster;
		uint32_t sector_size;
		uint32_t cluster_size;
		PartitionManager &partmanager;
};