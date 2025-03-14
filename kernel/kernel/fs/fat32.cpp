#include "kernel/datastr/buffer.hpp"
#include "kernel/datastr/vector.hpp"
#include "kernel/hal/job/diskjob.hpp"
#include "kernel/time/time.hpp"
#include <ctype.h>
#include <stdint.h>
#include <kernel/fs/fat32.hpp>
#include <kernel/sync/semaphore.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <stdio.h>
#include <string.h>
#include <kernel/kernel/log.hpp>
#include <kernel/cpp/min.hpp>

#define FAT_ERROR 0x0FFFFFF7
#define FAT_FINISH 0x0FFFFFF8
#define FAT_FREE 0

/// FAT Entry offsets
#define OFF_ENTRY_NAME 0
#define OFF_ENTRY_ATTR 11
// Reserved by windows NT
#define OFF_ENTRY_RESVNT 12
// Creation time in hundredths of a second
#define OFF_ENTRY_CREATTIME 13
#define OFF_ENTRY_CREATHOUR 14
#define OFF_ENTRY_CREATDATE 16
#define OFF_ENTRY_LASTACCDATE 18
#define OFF_ENTRY_HIGH16 20
#define OFF_ENTRY_LASTMODTIME 22
#define OFF_ENTRY_LASTMODDATE 24
#define OFF_ENTRY_LOW16 26
#define OFF_ENTRY_SIZE 28

#define ENTRY_SIZE 32

FAT32::FAT32(PartitionManager &partmanager, size_t partid) : partmanager(partmanager), 
	sector_size(hal::DiskManager::get().get_driver(partmanager.get_disk_id())->get_sector_size()), 
	fat_boot_buffer(sector_size) {
	this->partid = partid;
	fat_boot = reinterpret_cast<fat_BS_t*>(static_cast<uint8_t*>(fat_boot_buffer));
	if(disk_op(fat_boot_buffer, 0, 1, false)) {
		fat_extBS_32_t *fat_boot_ext_32 = reinterpret_cast<fat_extBS_32_t*>(&fat_boot->extended_section);
		total_sectors = (fat_boot->total_sectors_16 == 0)? fat_boot->total_sectors_32 : fat_boot->total_sectors_16;
		fat_size = (fat_boot->table_size_16 == 0)? fat_boot_ext_32->table_size_32 : fat_boot->table_size_16;
		memcpy(partname, fat_boot_ext_32->volume_label, 11);
		partname[11] = 0;
		fsinfo_sector = fat_boot_ext_32->fat_info;
		first_data_sector = fat_boot->reserved_sector_count + (fat_boot->table_count * fat_size);
		first_fat_sector = fat_boot->reserved_sector_count;
		n_data_sectors = total_sectors - (fat_boot->reserved_sector_count + (fat_boot->table_count * fat_size));
		total_clusters = n_data_sectors / fat_boot->sectors_per_cluster;
		root_cluster = fat_boot_ext_32->root_cluster;
		cluster_size = fat_boot->sectors_per_cluster*sector_size;
		log(INFO,"Total sectors: %d. FAT Size: %d. Cluster size: %d. Name: %s", total_sectors, fat_size, fat_boot->sectors_per_cluster, partname);
		log(INFO,"Total clusters: %d. First FAT sector: %d. Number of data sectors: %d. Root cluster: %d", total_clusters, first_fat_sector, n_data_sectors, root_cluster);
		log(INFO, "First data sector: %d", first_data_sector);
	} else {
		log(ERROR, "Could not load FAT from partition %d", partid);
		return;
	}
}

FAT32::~FAT32() {
}

bool FAT32::disk_op(uint8_t* buffer, uint64_t sector, uint32_t sector_count, bool write) {
	auto lba = partmanager.get_lba(partid, sector);
	volatile hal::DiskJob job{buffer, lba, sector_count, write};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	return job.state == hal::DiskJob::FINISHED;
}

uint32_t FAT32::next_cluster(uint32_t active_cluster) {
	unsigned int fat_offset = active_cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	Buffer<uint8_t> fat_buffer(sector_size);
	
	if(disk_op(fat_buffer, fat_sector, 1, false)) {
		uint32_t table_value = *(uint32_t*)&fat_buffer[ent_offset];
		table_value &= 0x0FFFFFFF;
		if(table_value >= FAT_FINISH) {
			log(INFO,"Chain finished");
		} else if(table_value == FAT_ERROR) {
			log(INFO,"Bad sector");
			return -1;
		} else {
			log(INFO,"Next cluster: %p", table_value);
		}
		return table_value;
	}
	return -1;
}

uint32_t FAT32::get_sector_of_cluster(uint32_t cluster) {
	return ((cluster - 2) * fat_boot->sectors_per_cluster) + first_data_sector;
}

bool FAT32::load_cluster(uint32_t cluster, uint8_t* buffer) {
	auto sector = get_sector_of_cluster(cluster);
	return disk_op(buffer, sector, cluster_size / sector_size, false);
}

bool FAT32::save_cluster(uint32_t cluster, uint8_t* buffer) {
	auto sector = get_sector_of_cluster(cluster);
	return disk_op(buffer, sector, cluster_size / sector_size, true);
}

uint32_t FAT32::search_free_cluster(uint32_t search_from) {
	log(INFO, "Trying to look at %d", search_from);
	unsigned int fat_offset = search_from * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	const uint32_t max_fat_sector = first_fat_sector + fat_size;
	Buffer<uint8_t> fat_buffer(sector_size);
	uint32_t free_cluster = -1;

	log(INFO, "Base FAT sector: %d, Max FAT sector: %d", fat_sector, max_fat_sector);

	for(size_t i = fat_sector; i < max_fat_sector; i++) {
		uint32_t base_cluster_entry_for_fat = (i - first_fat_sector) * (sector_size/4);
		log(INFO, "Base cluster: %p", base_cluster_entry_for_fat);

		if(!disk_op(fat_buffer, i, 1, false)) return -1;

		for(size_t j = (ent_offset/4); j < (sector_size / 4); j++) {
			const uint32_t* entry = reinterpret_cast<uint32_t*>(fat_buffer + 4*j);
			const auto cluster = base_cluster_entry_for_fat + j;
			if(cluster == search_from) continue;
			if(*entry == 0) {
				free_cluster = cluster;
				log(INFO, "Found free cluster: %d", free_cluster);
				return free_cluster;
			}
		}
		ent_offset = 0;
	}
	return -1;
}

bool FAT32::update_fsinfo(int diffclusters) {
	Buffer<uint8_t> buffer(sector_size);
	// Load FSInfo sector
	if(!disk_op(buffer, fsinfo_sector, 1, false)) return false;
	// Update free sectors count
	uint32_t* sectorscount = reinterpret_cast<uint32_t*>(buffer + 0x1e8);
	*sectorscount += diffclusters;
	// Save FSInfo
	return disk_op(buffer, fsinfo_sector, 1, true);
}

uint32_t FAT32::get_lookup_cluster(uint8_t* buffer) {
	// Load FSInfo sector
	if(!disk_op(buffer, fsinfo_sector, 1, false)) return false;
	// If look_from_fsinfo is 0xFFFFFFFF we need to comput it manually (look_from = root cluster)
	// Else we can try to search from that cluster in particular
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	uint32_t look_from = root_cluster;
	if(*look_from_fsinfo != 0xFFFFFFFF && *look_from_fsinfo < (n_data_sectors / fat_boot->sectors_per_cluster)) {
		look_from = *look_from_fsinfo;
	}
	return look_from;
}

bool FAT32::save_lookup_cluster(uint32_t cluster, uint8_t* buffer) {
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	*look_from_fsinfo = cluster;
	// Save FSInfo with lookup cluster updated
	return disk_op(buffer, fsinfo_sector, 1, true);
}

bool FAT32::alloc_clusters(uint32_t prevcluster, uint32_t nclusters) {
	// Allocate needed clusters
	Vector<uint32_t> clustervec;
	if(alloc_clusters(nclusters, clustervec)) {
		// Set the first allocated cluster of the chain to the previous cluster
		return set_next_cluster(prevcluster, clustervec[0]);
	}
	return false;
}

bool FAT32::alloc_clusters(uint32_t nclusters, Vector<uint32_t>& vec) {
	Buffer<uint8_t> buffer(sector_size);
	auto look_from = get_lookup_cluster(buffer);
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	
	for(size_t i = 0; i < nclusters; i++) {
		auto free_cluster = search_free_cluster(look_from);
		log(INFO, "Next free cluster from %p: %p", look_from, free_cluster);
		look_from = free_cluster;
		vec.push_back(free_cluster);
	}

	
	for(int i = 0; i < (vec.size() - 1); i++) {
		set_next_cluster(vec[i], vec[i+1]);
		log(INFO, "Set link %p -> %p", vec[i], vec[i+1]);
	}

	set_next_cluster(vec.back(), FAT_FINISH);

	auto next_free = search_free_cluster(look_from);
	save_lookup_cluster(next_free, buffer);

	return true;
}

uint32_t FAT32::first_cluster_for_directory(const char* dir_path) {
	log(INFO, "FAT32::first_cluster_for_directory(%s)", dir_path);
	int ndirs;
	char** directories = substr(dir_path, '/', &ndirs);
	if(!directories) return -1;
	uint32_t dir_cluster = root_cluster;
	Buffer<uint8_t> buffer(cluster_size);
	for(size_t i = 0; i < ndirs; i++) {
		if(strcmp(directories[i], "") == 0) continue;
		log(INFO, "Looking at directory %s", directories[i]);
		do {
			log(INFO, "Loading cluster %d", dir_cluster);
			load_cluster(dir_cluster, buffer);
			uint32_t cluster;
			if((cluster = match_cluster(buffer, directories[i], FAT_FLAGS::DIRECTORY, nullptr)) != -1) {
				dir_cluster = cluster;
				break;
			} else {
				dir_cluster = next_cluster(dir_cluster);
			}
		} while(dir_cluster < FAT_ERROR);
		if(dir_cluster >= FAT_ERROR) break;
	}

	for(size_t i = 0; i < ndirs; i++)
		kfree(directories[i]);
	kfree(directories);
	return dir_cluster;
}

uint32_t FAT32::cluster_for_filename(const char* filename, unsigned offset) {
	uint32_t cluster = find(filename, FAT_FLAGS::ARCHIVE);
	uint32_t cluster_offset = offset/cluster_size;
	for(size_t i = 0; i < cluster_offset; i++) {
		cluster = next_cluster(cluster);
	}

	return cluster;
}

bool FAT32::filecmp(const char* basename, const char* entrydata, bool lfn) {
	log(INFO, "%s == %s", basename, entrydata);
	if(lfn) {
		/// TODO: support for UTF-16 LFN entries
		return strcmp(basename, entrydata);
	} else {
		char name[13];
		memset(name, 0, sizeof(name));
		size_t i;
		// Copy filename
		memcpy(name, entrydata, 8);
		// Remove trailing spaces
		for(i = 7; i >= 0; i--) {
			if(name[i] != ' ') break;
			name[i] = 0;
		}
		bool dotflag = false;
		// Copy extension
		for(size_t j = 8; j < 11; j++, i++) {
			if(entrydata[j] != ' ') {
				if(!dotflag) {
					name[i++] = '.';
					dotflag = true;
				}
				name[i] = entrydata[j];
			}
		}
		return strcasecmp(basename, name);
	}
}

off_t FAT32::read(const char* filename, unsigned offset, unsigned nbytes, char* buffer) {
	auto cluster = cluster_for_filename(filename, offset);
	log(INFO, "Cluster for %s: %d", filename, cluster);
	if(cluster < FAT_ERROR) {
		off_t readbytes = 0;
		auto incluster_offset = offset % cluster_size;
		auto incluster_nbytes = nbytes > (cluster_size - incluster_offset) ? (cluster_size - incluster_offset) : nbytes;
		const unsigned clusters = (incluster_offset + nbytes + cluster_size - 1) / cluster_size;
		Buffer<uint8_t> diskbuf(cluster_size);
		for(size_t i = 0; cluster < FAT_ERROR && i < clusters; i++, cluster = next_cluster(cluster)) {
			load_cluster(cluster, diskbuf);
			memcpy(buffer, diskbuf+incluster_offset, incluster_nbytes);
			buffer += incluster_nbytes;
			nbytes -= incluster_nbytes;
			readbytes += incluster_nbytes;
			incluster_offset = 0;
			incluster_nbytes = nbytes > cluster_size ? cluster_size : nbytes;
		}
		return readbytes;
	}
	return -1;
}

bool FAT32::set_next_cluster(uint32_t cluster, uint32_t next_cluster) {
	unsigned int fat_offset = cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	Buffer<uint8_t> fat_buffer(sector_size);

	if(disk_op(fat_buffer, fat_sector, 1, false)) {
		uint32_t* table_value = (uint32_t*)&fat_buffer[ent_offset];
		auto prev_table_value = *table_value;
		// MS spec says to leave the 4 upper bits as they were
		*table_value = (next_cluster & 0x0FFFFFFF) | (prev_table_value & 0xF0000000);
		// Update first fat table
		if(!disk_op(fat_buffer, fat_sector, 1, true)) return false;
		// Update second fat table
		return disk_op(fat_buffer, fat_sector + fat_size, 1, true);
	}
	return false;
}

off_t FAT32::write(const char* filename, unsigned offset, unsigned nbytes, const char* buffer) {
	struct stat buf;
	int ret = stat(filename, &buf);
	if(ret == -1) return -1;
	
	if(offset + nbytes > buf.st_size) {
		truncate(filename, offset + nbytes);
		buf.st_size = offset + nbytes;
	}
	auto filecluster = cluster_for_filename(filename, offset);
	if(filecluster < FAT_ERROR) {
		off_t readbytes = 0;
		auto incluster_offset = offset % cluster_size;
		auto incluster_nbytes = nbytes > (cluster_size - incluster_offset) ? (cluster_size - incluster_offset) : nbytes;
		const unsigned clusters = (incluster_offset + nbytes + cluster_size - 1) / cluster_size;
		Buffer<uint8_t> diskbuf(cluster_size);
		for(size_t i = 0; filecluster < FAT_ERROR && i < clusters; i++, filecluster = next_cluster(filecluster)) {
			load_cluster(filecluster, diskbuf);
			memcpy(diskbuf+incluster_offset, buffer, incluster_nbytes);
			buffer += incluster_nbytes;
			nbytes -= incluster_nbytes;
			readbytes += incluster_nbytes;
			incluster_offset = 0;
			incluster_nbytes = nbytes > cluster_size ? cluster_size : nbytes;
			save_cluster(filecluster, diskbuf);
		}
		delete[] diskbuf;

		return readbytes;
	}
	return -1;
}

off_t FAT32::truncate(const char* filename, unsigned nbytes) {
	struct stat buf;
	int ret = stat(filename, &buf);
	if(ret == -1) return -1;
	auto filecluster = cluster_for_filename(filename, 0);
	if(filecluster == -1) return -1;
	uint32_t current_size_in_clusters = (static_cast<uint32_t>(buf.st_size) + sector_size - 1) / cluster_size;
	auto new_size_in_clusters = (nbytes + sector_size -1) / cluster_size;

	bool zerosize = false;
	if(new_size_in_clusters == 0) {
		if(current_size_in_clusters == 0) return 0;
		Vector<uint32_t> clusters;
		get_all_clusters_from(filecluster, clusters);
		// Mark each and every cluster of the file as empty
		for(size_t i = 0; i < clusters.size(); i++) {
			set_next_cluster(clusters[i], FAT_FREE);
		}
		zerosize = true;
		if(!update_fsinfo(clusters.size())) return 0;
	} else if(current_size_in_clusters > new_size_in_clusters) {
		log(INFO, "Current size in clusters (%d) less than new size in clusters (%d)", current_size_in_clusters, new_size_in_clusters);
		Vector<uint32_t> clusters;
		get_all_clusters_from(filecluster, clusters);
		set_next_cluster(clusters[new_size_in_clusters - 1], FAT_FINISH);
		for(size_t i = current_size_in_clusters - new_size_in_clusters; i < clusters.size(); i++) {
			// Mark as free
			set_next_cluster(clusters[i], FAT_FREE);
		}
		log(INFO, "Size change from %d to %d sectors", current_size_in_clusters, new_size_in_clusters);
		if(!update_fsinfo(current_size_in_clusters - new_size_in_clusters)) return 0;
	} else if (current_size_in_clusters < new_size_in_clusters) {
		if(current_size_in_clusters == 0) {
			filecluster = search_free_cluster(2);
			if(filecluster == -1) return 0;
			struct stat buf {
				0,
				TimeManager::get().get_time(),
				TimeManager::get().get_time(),
				0,
				filecluster
			};
			file_setproperty(filename, &buf);
			if(new_size_in_clusters - 1 == 0) {
				set_next_cluster(filecluster, FAT_FINISH);
			} else {
				if(!alloc_clusters(filecluster, new_size_in_clusters -1)) return 0;
			}
		} else {
			auto last_filecluster = last_cluster_in_chain(filecluster);
			if(!alloc_clusters(last_filecluster, new_size_in_clusters - current_size_in_clusters)) return 0;
		}
		if(!update_fsinfo(current_size_in_clusters - new_size_in_clusters)) return 0;
	}
	// Update the size of the file.
	struct stat properties{
		nbytes,
		TimeManager::get().get_time(),
		TimeManager::get().get_time(),
		0,
		static_cast<ino_t>(zerosize ? 0 : -1)
	};
	file_setproperty(filename, &properties);
	return nbytes;
}

bool FAT32::file_setproperty(const char* filename, const struct stat* properties) {
	int nentry = -1;
	uint32_t dir_cluster;
	auto found = find(filename, FAT_FLAGS::ARCHIVE, nullptr, &nentry, 0, &dir_cluster);
	if(found >= FAT_ERROR) {
		return false;
	}
	return setstat(dir_cluster, nentry, properties);
}

void FAT32::get_all_clusters_from(uint32_t cluster, Vector<uint32_t> &clusters) {
	do {
		clusters.push_back(cluster);
		cluster = next_cluster(cluster);
	} while(cluster < FAT_ERROR);
}

uint32_t FAT32::last_cluster_in_chain(uint32_t cluster) {
	uint32_t last_cluster = -1;
	do {
		last_cluster = cluster;
		cluster = next_cluster(cluster);
	} while(cluster < FAT_ERROR);
	return last_cluster;
}

uint16_t date2fatdate(const date &dt) {
	uint16_t datecalc = 0;
	datecalc |= (dt.year & 0x7F) << 9;
	datecalc |= (dt.month & 0xF) << 5;
	datecalc |= (dt.day & 0x1F);
	return datecalc;
}

uint16_t date2fattime(const date& dt) {
	uint16_t timecalc = 0;
	timecalc |= (dt.year & 0x1F) << 11;
	timecalc |= (dt.month & 0x3F) << 5;
	timecalc |= ((dt.second/2) & 0x1F);
	return timecalc;
}

bool FAT32::setstat(uint32_t dir, int nentry, const struct stat* properties) {
	Buffer<uint8_t> buffer(cluster_size);
	load_cluster(dir, buffer);
	uint8_t* ptr = buffer + ENTRY_SIZE*nentry;
	set_sfn_entry_data(ptr, nullptr, FAT_FLAGS::NONE, properties);
	return save_cluster(dir, buffer);
}

uint32_t FAT32::get_parent_dir_cluster(const char* filename, const char* basename) {
	Buffer<char> dir_path{static_cast<size_t>(basename - filename + 1)};
	memset(dir_path, 0, dir_path.get_size());
	memcpy(dir_path, filename, basename - filename);
	dir_path[basename - filename] = 0;
	log(INFO, "Dir path: %s", dir_path.get_data());
	log(INFO, "Basename: %s", basename);
	return first_cluster_for_directory(dir_path);
}

int FAT32::stat(const char* filename, struct stat* buf) {
	auto found = find(filename, FAT_FLAGS::ARCHIVE, buf);
	if(found >= FAT_ERROR) return -1;
	return 0;
}

uint32_t FAT32::match_cluster(uint8_t* buffer, const char* basename, FAT_FLAGS flags, struct stat* buf, int* nentry, int nfree) {
	uint8_t lfn_order = 0;
	char lfn_buffer[256] = {0};
	bool has_lfn = false;
	const size_t entries_per_cluster = cluster_size / ENTRY_SIZE;
	const int free = nfree;
	for(size_t i = 0; i < entries_per_cluster; i++) {
		char name[14];
		memset(name, 0, sizeof(name));
		uint8_t* ptr_to_i = buffer + (ENTRY_SIZE*i);
		FAT_FLAGS attr = static_cast<FAT_FLAGS>(ptr_to_i[11]);
		if(attr == FAT_FLAGS::LFN) {
			nfree = free;
			has_lfn = true;
			uint8_t pos = *ptr_to_i & 0x3F;
			lfn_order = (pos - 1) * 13;
			// First 5 2-byte characters of the entry
			uint16_t *first5 = reinterpret_cast<uint16_t*>(ptr_to_i + 1);
			uint16_t *second6 = reinterpret_cast<uint16_t*>(ptr_to_i + 14);
			uint16_t *third2 = reinterpret_cast<uint16_t*>(ptr_to_i + 28);
			for (int j = 0; j < 5; j++) lfn_buffer[lfn_order + j] = first5[j];
			for (int j = 0; j < 6; j++) lfn_buffer[lfn_order + j + 5] = second6[j];
			for (int j = 0; j < 2; j++) lfn_buffer[lfn_order + j + 11] = third2[j];
	
			lfn_order += 13;
		} else if(attr != FAT_FLAGS::NONE) {
			nfree = free;
			char sfn_name[12] = {0};
			memcpy(sfn_name, ptr_to_i, 11);
			sfn_name[11] = 0;
			log(INFO, "%s %s", sfn_name, lfn_buffer);
			// If we match return
			if((static_cast<uint8_t>(attr) & static_cast<uint8_t>(flags)) && !filecmp(basename, has_lfn ? lfn_buffer : sfn_name, has_lfn)) {
				log(INFO, "looks like we have a match");
				if(buf) {
					direntrystat(ptr_to_i, buf);
				}
				if(nentry) {
					*nentry = i;
				}
				return get_cluster_from_direntry(ptr_to_i);
			}
			memset(lfn_buffer, 0, sizeof(lfn_buffer));
		} else {
			if(nentry && !strcmp(basename, "")) {
				nfree--;
				if(nfree == 0) {
					*nentry = i - free + 1;
					return i;
				}
			}
		}
	}
	return -1;
}

void FAT32::direntrystat(uint8_t* direntry, struct stat *buf, FAT_FLAGS* flags) {
	uint16_t creattime = *reinterpret_cast<uint16_t*>(direntry + 14);
	// Format: 0000 0|000 000|0 0000
	date creatd;
	creatd.second = (creattime & 0x1f) * 2;
	creatd.minute = (creattime & 0x7E0) >> 5;
	creatd.hour = (creattime & 0xF800) >> 11;

	// Format: 0000 000|0 000|0 0000
	uint16_t creatdate = *reinterpret_cast<uint16_t*>(direntry + 16);
	creatd.day = (creatdate & 0x1f);
	creatd.month = (creatdate & 0x1E0) >> 5;
	// 1980 is the FAT epoch
	creatd.year = ((creatdate & 0xFE00) >> 9) + 1980;

	buf->st_ctime = TimeManager::date_to_timestamp(creatd);

	log(INFO, "%d-%d-%d %d:%d:%d", creatd.year, creatd.month, creatd.day, creatd.hour, creatd.minute, creatd.second);

	uint16_t modtime = *reinterpret_cast<uint16_t*>(direntry + 22);
	// Format: 0000 0|000 000|0 0000
	date moddt;
	moddt.second = (creattime & 0x1f) * 2;
	moddt.minute = (creattime & 0x7E0) >> 5;
	moddt.hour = (creattime & 0xF800) >> 11;
	uint16_t moddate = *reinterpret_cast<uint16_t*>(direntry + 24);
	moddt.day = (moddate & 0x1f);
	moddt.month = (moddate & 0x1E0) >> 5;
	moddt.year = ((moddate & 0xFE00) >> 9) + 1980;
	buf->st_mtime = TimeManager::date_to_timestamp(moddt);

	// Format: 0000 000|0 000|0 0000
	uint16_t accdate = *reinterpret_cast<uint16_t*>(direntry + 18);
	date accdt;
	accdt.day = (accdate & 0x1f);
	accdt.month = (accdate & 0x1E0) >> 5;
	accdt.year = ((accdate & 0xFE00) >> 9) + 1980;
	buf->st_atime = TimeManager::date_to_timestamp(accdt);

	buf->st_size = *reinterpret_cast<uint32_t*>(direntry + OFF_ENTRY_SIZE);

	buf->st_ino = get_cluster_from_direntry(direntry);
	if(flags) *flags = static_cast<FAT_FLAGS>(direntry[OFF_ENTRY_ATTR]);
}

uint32_t FAT32::get_cluster_from_direntry(uint8_t* direntry) {
	uint16_t low_16 = *reinterpret_cast<uint16_t*>(direntry + 26);
	uint16_t high_16 = *reinterpret_cast<uint16_t*>(direntry + 20);
	uint32_t cluster = (high_16 << 16) | low_16;
	return cluster;
}

uint32_t FAT32::create_entry(uint8_t* buffer, const char* filename, FAT_FLAGS flags, uint32_t* parent_dircluster, uint32_t* first_parent_dircluster) {
	const char* basenameptr = rfind(filename, '/');
	if(!basenameptr) return -1;
	const uint32_t first_dir_cluster = get_parent_dir_cluster(filename, basenameptr);
	uint32_t dir_cluster = first_dir_cluster;
	basenameptr++;

	if(dir_cluster < FAT_ERROR) {
		/// Check if there is going to be enough size.
		char basename[256];
		memset(basename, 0, sizeof(basename));
		const size_t ndirentries = cluster_size / ENTRY_SIZE;
		size_t basenamelen = strlen(basenameptr);
		if(basenamelen > 255) return false;
		strncpy(basename, basenameptr, basenamelen);
		basename[basenamelen] = 0;
		// Round up (+12) + another required entry (non-LFN entry)
		const size_t nreqentries = ((basenamelen + 12 +13) / 13);
		if(nreqentries > ndirentries) {
			log(ERROR, "Not enough space in a directory for the file");
			return -1;
		}
		
		int nentry = -1;
		uint32_t last_cluster = find(buffer, nullptr, FAT_FLAGS::NONE, nullptr, &nentry, nreqentries, &dir_cluster);
		//if(last_cluster >= FAT_ERROR) return false;

		log(INFO, "Reported nentry: %d", nentry);
		
		if(nentry == -1) { /// No free space in the directory. We need to allocate more clusters
			last_cluster = last_cluster_in_chain(first_dir_cluster);
			if(!alloc_clusters(last_cluster, 1)) return -1; /// Could not allocate more clusters
			last_cluster = next_cluster(last_cluster);
			load_cluster(last_cluster, buffer);
			dir_cluster = last_cluster;
			memset(buffer, 0, cluster_size);
			update_fsinfo(-1);
			nentry = 0;
		}
		log(INFO, "Max offset: %d", ENTRY_SIZE*(nentry + nreqentries - 1));
		// SFN entry
		uint8_t* ptr = buffer + ENTRY_SIZE*(nentry + nreqentries - 1);
		struct stat properties {
			0,
			TimeManager::get().get_time(),
			TimeManager::get().get_time(),
			TimeManager::get().get_time(),
			0
		};
		// Data for LFN entry
		auto checksum = set_sfn_entry_data(ptr, basename, flags, &properties);
		// Christ in a handbasket, IT'S BACKWARDS
		for(size_t i = nentry, nentries = nreqentries - 1; i < nentry + nreqentries - 1; i++, nentries--) {
			uint8_t* ptr = buffer + ENTRY_SIZE*i;
			const int order = nentries;
			const char* basenameptr = basename + ((order - 1) * 13);
			set_lfn_entry_data(ptr, basenameptr, order, i == nentry, checksum);
		}			
		save_cluster(dir_cluster, buffer);
		log(INFO, "Created file %s", filename);
		if(dir_cluster >= FAT_ERROR) return -1;
		if(parent_dircluster) {
			*parent_dircluster = dir_cluster;
		}
		if(first_parent_dircluster) *first_parent_dircluster = first_dir_cluster;
		return nentry + nreqentries - 1;
	}
	return -1;
}

bool FAT32::touch(const char* filename) {
	struct stat buf;
	// Check whether file already exists
	int ret = stat(filename, &buf);

	if(ret == -1) {
		Buffer<uint8_t> buf(cluster_size);
		return create_entry(buf, filename, FAT_FLAGS::ARCHIVE) != -1;
	}
	return false;
}

bool FAT32::mkdir(const char* directory) {
	struct stat stbuf;
	// Check whether file already exists
	int ret = stat(directory, &stbuf);

	if(ret == -1) {
		Buffer<uint8_t> buf(cluster_size);
		uint32_t parentdir;
		uint32_t first_parentdir;
		uint32_t entry = create_entry(buf, directory, FAT_FLAGS::DIRECTORY, &parentdir, &first_parentdir);
		if(entry == -1) return false;
		uint8_t* entryptr = buf + ENTRY_SIZE*entry;
		Vector<uint32_t> clustervec;
		// Create a new cluster that will hold the newly created directory data
		if(alloc_clusters(1, clustervec)) {
			log(INFO, "Returned: %p", clustervec[0]);
			struct stat properties {
				0,
				TimeManager::get().get_time(),
				TimeManager::get().get_time(),
				TimeManager::get().get_time(),
				clustervec[0]
			};
			set_sfn_entry_data(entryptr, NULL, FAT_FLAGS::NONE, &properties);
			save_cluster(parentdir, buf);
			Buffer<uint8_t> clusterbuffer(cluster_size);
			memset(clusterbuffer, 0, clusterbuffer.get_size());

			// . entry
			set_sfn_entry_data(clusterbuffer, ".", FAT_FLAGS::DIRECTORY, &properties);
			properties.st_ino = first_parentdir;
			// .. entrycreate_entry(, const char *filename, FAT_FLAGS flags)
			set_sfn_entry_data(clusterbuffer + ENTRY_SIZE, "..", FAT_FLAGS::DIRECTORY, &properties);

			update_fsinfo(-1);

			set_next_cluster(clustervec[0], FAT_FINISH);

			// The cluster should have necessary data
			return save_cluster(clustervec[0], clusterbuffer);
		} else {
			return false;
		}
	}
	return false;
}

void FAT32::set_lfn_entry_data(uint8_t* ptr, const char* basename, uint8_t order, bool is_last, uint8_t checksum) {
	ptr[0] = order;
	ptr[11] = static_cast<uint8_t>(FAT_FLAGS::LFN);
	
	uint8_t* first_5 = reinterpret_cast<uint8_t*>(ptr + 1);
	for(size_t i = 0; i < 5; i++) {
		first_5[2*i] = basename[i];
		first_5[2*i + 1] = 0;
	}
	uint8_t* second_6 = reinterpret_cast<uint8_t*>(ptr + 14);
	for(size_t i = 0; i < 6; i++) {
		second_6[2*i] = basename[5 + i];
		second_6[2*i + 1] = 0;
	}

	uint8_t* third_2 = reinterpret_cast<uint8_t*>(ptr + 28);
	for(size_t i = 0; i < 2; i++) {
		third_2[2*i] = basename[11 + i];
		third_2[2*i + 1] = 0;
	}

	ptr[13] = checksum;

	// If last entry mark it as such
	if(is_last) {
		ptr[0] |= 0x40;
	}
}

uint8_t FAT32::checksum_sfn(const char* basename) {
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++) {
        sum = ((sum >> 1) | (sum << 7)) + basename[i];
    }
    return sum;
}

uint8_t FAT32::set_sfn_entry_data(uint8_t* ptr, const char* basename, FAT_FLAGS flags, const struct stat* properties) {
	uint8_t checksum = 0;
	if(properties) {
		if(properties->st_atime != -1) {
			date dt = TimeManager::timestamp_to_date(properties->st_atime);
			uint16_t* date = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_LASTACCDATE);
			auto datecalc = date2fatdate(dt);

			*date = datecalc;
		}
		if(properties->st_mtime != -1) {
			date dt = TimeManager::timestamp_to_date(properties->st_mtime);
			uint16_t* date = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_LASTMODDATE);
			auto datecalc = date2fatdate(dt);
			*date = datecalc;

			uint16_t* time = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_LASTMODTIME);
			auto timecalc = date2fattime(dt);
			*time = timecalc;
		}
		if(properties->st_ctime != -1) {
			date dt = TimeManager::timestamp_to_date(properties->st_mtime);
			uint16_t* date = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_CREATDATE);
			auto datecalc = date2fatdate(dt);
			*date = datecalc;

			uint16_t* time = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_CREATTIME);
			auto timecalc = date2fattime(dt);
			*time = timecalc;
		}
		if(properties->st_size != -1) {
			uint32_t* sizeptr = reinterpret_cast<uint32_t*>(ptr + OFF_ENTRY_SIZE);
			*sizeptr = properties->st_size;
		}
		if(properties->st_ino != -1) {
			uint16_t* low_clnumber = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_LOW16);
			uint16_t* high_clnumber = reinterpret_cast<uint16_t*>(ptr + OFF_ENTRY_HIGH16);

			*low_clnumber = properties->st_ino;
			*high_clnumber = (properties->st_ino >> 16);
		}
	}
	if(basename) {
		char uppbasename[11];
		/// Default padding for short filenames are spaces
		memset(uppbasename, 0x20, sizeof(uppbasename));
		// Find the .
		const char* dot = rfind(basename, '.');
		if(!strcmp(basename, ".") || !strcmp(basename, "..")) {
			memcpy(uppbasename, basename, strlen(basename));
		} else if(dot) {
			// Copy the extension into the name
			memcpy(uppbasename + 8, dot + 1, 3);
			// Copy either first 6 chars or until the dot
			if(dot - basename > 6) {
				memcpy(uppbasename, basename, 6);
			} else {
				memcpy(uppbasename, basename, dot - basename);
			}
			const unsigned last_char = dot - basename > 6 ? 6 : dot - basename;
			// Copy a ~ and a random number into the last character
			int random = ((rand() % 10) + 10) % 10;
			uppbasename[last_char] = '~';
			uppbasename[last_char+1] = random + '0';
		} else {
			// Copy first 8 characters into name
			memcpy(uppbasename, basename, 8);
		}
		for(size_t i = 0; i < 11; i++) {
			uppbasename[i] = toupper(uppbasename[i]);
		}
		memcpy(ptr, uppbasename, sizeof(uppbasename));
		checksum = checksum_sfn(uppbasename);
	}
	if(flags != FAT_FLAGS::NONE) {
		ptr[OFF_ENTRY_ATTR] = static_cast<uint8_t>(flags);
	}
	return checksum;
}

uint32_t FAT32::remove_entry(uint8_t* buffer, int nentry, uint8_t* entry) {
	// Find whether there are lfn entries
	bool found_first = false;
	int i;
	for(i = nentry - 1; i >= 0; i--) {
		uint8_t* ptr = buffer + ENTRY_SIZE*i;
		if(ptr[OFF_ENTRY_ATTR] == static_cast<uint8_t>(FAT_FLAGS::LFN) && (ptr[0] & 0x40)) {
			found_first = true;
			break;
		}
	}
	if(found_first) {
		for(int j = i; j < nentry; j++) {
			// Delete LFN entries
			uint8_t* ptr = buffer + ENTRY_SIZE*j;
			memset(ptr, 0, ENTRY_SIZE);
		}
	}
	uint8_t* ptr = buffer + ENTRY_SIZE*nentry;
	uint32_t cluster =  get_cluster_from_direntry(ptr);
	if(entry) memcpy(entry, ptr, ENTRY_SIZE);
	// Delete the entry
	memset(ptr, 0, ENTRY_SIZE);
	return cluster;
}

uint32_t FAT32::find(uint8_t* buffer, const char* path, FAT_FLAGS flags, struct stat* statbuf, int* nentry, int nfree, uint32_t* dir_cluster, uint32_t* prev_dir_cluster) {
	const char* basename = path ? rfind(path, '/') : "";
	if(!basename) return -1;
	uint32_t dir = path ? get_parent_dir_cluster(path, basename) : *dir_cluster;
	if(path) basename++;
	if(dir >= FAT_ERROR) return false;

	uint32_t cluster;
	uint32_t prev_cluster = -1;
	do {
		log(INFO, "Loading cluster %d", dir);
		load_cluster(dir, buffer);
		if((cluster = match_cluster(buffer, basename, flags, statbuf, nentry, nfree)) != -1) {
			break;
		}
		prev_cluster = dir;
		dir = next_cluster(dir);
	} while(dir < FAT_ERROR);
	if(dir_cluster) *dir_cluster = dir;
	if(prev_dir_cluster) *prev_dir_cluster = prev_cluster;
	return dir >= FAT_ERROR ? dir : cluster;
}

uint32_t FAT32::find(const char* path, FAT_FLAGS flags, struct stat* statbuf, int* nentry, int nfree, uint32_t* dir_cluster, uint32_t* prev_dir_cluster) {
	return find(Buffer<uint8_t>(cluster_size), path, flags, statbuf, nentry, nfree, dir_cluster, prev_dir_cluster);
}

bool FAT32::remove(const char* path) {
	return remove_generic(path, FAT_FLAGS::ARCHIVE);
}

bool FAT32::remove_generic(const char* path, FAT_FLAGS flags) {
	Buffer<uint8_t> buf(cluster_size);
	int nentry = -1;
	uint32_t dir_cluster, prev_dir_cluster;
	// Search for the entry
	auto cluster = find(buf, path, flags, nullptr, &nentry, 0, &dir_cluster, &prev_dir_cluster);
	log(INFO, "Cluster returned: %d, directory cluster: %d", cluster, dir_cluster);
	if(cluster >= FAT_ERROR || nentry == -1) return false;
	// Remove the entry
	auto filecluster = remove_entry(buf, nentry);
	if(filecluster >= FAT_ERROR) return false;
	Vector<uint32_t> clusters;
	get_all_clusters_from(filecluster, clusters);
	for(size_t i = 0; i < clusters.size(); i++) {
		set_next_cluster(clusters[i], FAT_FREE);
	}
	update_fsinfo(clusters.size());
	if(!save_cluster(dir_cluster, buf)) return false;
	// Check if we're at the last cluster
	if(next_cluster(dir_cluster) >= FAT_ERROR) {
		// Check if there are no free entries left
		nentry = -1;
		cluster = find(buf, nullptr, FAT_FLAGS::NONE, nullptr, &nentry, cluster_size / ENTRY_SIZE, &dir_cluster);
		if(nentry != -1) { // We just deleted the last entry in the directory and the directory is free.
			// Previous directory doesn't exist
			if(prev_dir_cluster == -1) return true;
			// Delete directory cluster
			set_next_cluster(prev_dir_cluster, FAT_FINISH);
			set_next_cluster(dir_cluster, FAT_FREE);
			update_fsinfo(1);
		}
	}
	return true;
}

bool FAT32::is_dir_empty(const char* directory) {
	uint32_t cluster = first_cluster_for_directory(directory);
	if(cluster >= FAT_ERROR) return false;
	Buffer<uint8_t> buf(cluster_size);
	if(!load_cluster(cluster, buf)) return false;
	
	// Directory is empty if it has only one cluster and there are only
	// two entries: . and ..
	auto next = next_cluster(cluster);
	if(next < FAT_ERROR) return false;
	const int nentries_per_dir = cluster_size/ENTRY_SIZE;
	// Check if there are more entries available
	for(int i = 2; i < nentries_per_dir; i++) {
		uint8_t* entryptr = buf + ENTRY_SIZE*i;
		if(entryptr[OFF_ENTRY_ATTR] != 0) return false;
	}

	return true;
}

bool FAT32::rmdir(const char* directory) {
	if(is_dir_empty(directory)) {
		return remove_generic(directory, FAT_FLAGS::DIRECTORY);
	}
	return false;
}

bool FAT32::rename(const char* src, const char* dest) {
	constexpr FAT_FLAGS flags_to_search = static_cast<FAT_FLAGS>(static_cast<uint8_t>(FAT_FLAGS::DIRECTORY) | static_cast<uint8_t>(FAT_FLAGS::ARCHIVE));
	Buffer<uint8_t> buf(cluster_size);
	int nentry = -1;
	uint32_t dir_cluster, prev_dir_cluster;
	// Search for the entry
	auto cluster = find(buf, src, flags_to_search, nullptr, &nentry, 0, &dir_cluster, &prev_dir_cluster);
	log(INFO, "Cluster returned: %d, directory cluster: %d", cluster, dir_cluster);
	if(cluster >= FAT_ERROR || nentry == -1) return false;
	uint8_t entry[ENTRY_SIZE];
	// Remove the entry
	auto filecluster = remove_entry(buf, nentry, entry);
	if(filecluster >= FAT_ERROR) return false;
	struct stat statbuf;
	FAT_FLAGS entry_flags = FAT_FLAGS::NONE;
	direntrystat(entry, &statbuf, &entry_flags);

	// Now we create a new entry in the directory
	Buffer<uint8_t> createentrybuf(cluster_size);
	nentry = create_entry(createentrybuf, dest, entry_flags);
	if(nentry == -1) return false;

	const char* basename = rfind(dest, '/') + 1;
	
	set_sfn_entry_data(createentrybuf + ENTRY_SIZE*nentry, basename, FAT_FLAGS::NONE, &statbuf);
	return true;
}