#include <stdint.h>
#include <kernel/fs/partmanager.hpp>
#include "../arch/i386/defs/fs/fat32/bs.hpp"
#include "../arch/i386/defs/fs/fat32/entryflags.hpp"
#include "const.hpp"
#include "kernel/datastr/vector.hpp"
#include <kernel/datastr/buffer.hpp>

class FAT32 {
	public:
		FAT32(PartitionManager &partmanager, size_t partid);
		~FAT32();

		off_t read(const char* filename, unsigned offset, unsigned nbytes, char* buffer);
		off_t write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer);
		off_t truncate(const char* filename, unsigned nbytes);
		int stat(const char* filename, stat* buf);

		bool touch(const char* filename);

		bool mkdir(const char* directory);
		bool rmdir(const char* directory);
		
		bool rename(const char* src, const char* dest);
		bool remove(const char* path);

		DIR* opendir(const char* directory);
		dirent* readdir(DIR* dir);
		void closedir(DIR* dir);
	private:
		uint32_t get_sector_of_cluster(uint32_t cluster);
		uint32_t next_cluster(uint32_t active_cluster);
		bool set_next_cluster(uint32_t cluster, uint32_t next_cluster);
		bool load_cluster(uint32_t cluster, uint8_t* buffer);
		bool save_cluster(uint32_t cluster, uint8_t* buffer);
		uint32_t first_cluster_for_directory(const char* directory);
		uint32_t cluster_for_filename(const char* filename, unsigned offset);
		bool filecmp(const char* basename, const char* entrydata, bool lfn);
		uint32_t match_cluster(uint8_t* cluster, const char* basename, FAT_FLAGS flags, struct stat* statbuf = nullptr, int* nentry = nullptr, int nfree = 0);
		void direntrystat(uint8_t* direntry, struct stat* statbuf);
		bool update_fsinfo(int diffclusters);

		uint32_t get_cluster_from_direntry(uint8_t* direntry);
		
		bool file_setproperty(const char* filename, const struct stat* properties);
		bool setstat(uint32_t dircluster, int nentry, const struct stat* properties);
		bool alloc_clusters(uint32_t prevcluster, uint32_t nclusters);
		bool alloc_clusters(uint32_t nclusters, Vector<uint32_t>& clustervec);
		uint32_t search_free_cluster(uint32_t searchfrom);
		uint32_t get_lookup_cluster(uint8_t* buffer);
		bool save_lookup_cluster(uint32_t cluster, uint8_t* buffer);

		bool is_dir_empty(const char* directory);

		uint32_t get_parent_dir_cluster(const char* filename, const char* basename);
		uint8_t checksum_sfn(const char* basename);
		uint8_t set_sfn_entry_data(uint8_t* ptr, const char* basename, FAT_FLAGS flags, const struct stat* properties);
		void set_lfn_entry_data(uint8_t* ptr, const char* basename, uint8_t order, bool is_last, uint8_t checksum);
		uint32_t create_entry(uint8_t* buffer, 
			const char* filename, 
			FAT_FLAGS flags, 
			uint32_t* parent_dircluster = nullptr, 
			uint32_t* first_parent_dircluster = nullptr);

		uint32_t find(const char* path, 
			FAT_FLAGS flags, 
			struct stat* statbuf = nullptr, 
			int* nentry = nullptr, 
			int nfree = 0, 
			uint32_t* dir_cluster = nullptr, 
			uint32_t* prev_dir_cluster = nullptr);
		uint32_t find(uint8_t* buffer, 
			const char* path, 
			FAT_FLAGS flags, 
			struct stat* statbuf = nullptr, 
			int* nentry = nullptr, 
			int nfree = 0, 
			uint32_t* dir_cluster = nullptr,
			uint32_t* prev_dir_cluster = nullptr);

		uint32_t remove_entry(uint8_t* buffer, int nentry, uint8_t* entry = nullptr);

		bool disk_op(uint8_t* buffer, uint64_t sector, uint32_t sector_count, bool write);

		void get_all_clusters_from(uint32_t cluster, Vector<uint32_t> &clusters);
		uint32_t last_cluster_in_chain(uint32_t cluster);

		bool remove_generic(const char* path, FAT_FLAGS flags);

		size_t partid;
		char partname[12];
		
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

		Buffer<uint8_t> fat_boot_buffer;
		PartitionManager &partmanager;
};