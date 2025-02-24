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

#define FAT_ERROR 0x0FFFFFF7
#define FAT_FINISH 0x0FFFFFF8

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

FAT32::FAT32(PartitionManager &partmanager, size_t partid) : partmanager(partmanager), 
	sector_size(hal::DiskManager::get().get_driver(partmanager.get_disk_id())->get_sector_size()), 
	fat_boot_buffer(sector_size) {
	this->partid = partid;
	auto lba = partmanager.get_lba(partid, 0);

	volatile hal::DiskJob job{fat_boot_buffer, lba, 1, 0};
	hal::DiskManager::get().sleep_job(0, &job);
	fat_boot = reinterpret_cast<fat_BS_t*>(static_cast<uint8_t*>(fat_boot_buffer));
	if(job.state == hal::DiskJob::FINISHED) {
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
		log(ERROR, "Could not load FAT from lba %d", lba);
		return;
	}
}

FAT32::~FAT32() {
}

uint32_t FAT32::next_cluster(uint32_t active_cluster) {
	unsigned int fat_offset = active_cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	Buffer<uint8_t> fat_buffer(sector_size);
	
	auto lba = partmanager.get_lba(partid, fat_sector);
	volatile hal::DiskJob job{fat_buffer, lba, 1, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::FINISHED) {
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
	auto lba = partmanager.get_lba(partid, sector);
	volatile hal::DiskJob job{buffer, lba, fat_boot->sectors_per_cluster, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);

	return job.state == hal::DiskJob::FINISHED;
}

uint32_t FAT32::search_free_cluster(uint32_t search_from) {
	log(INFO, "Trying to look at %d", search_from);
	unsigned int fat_offset = search_from * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	uint32_t max_fat_sector = first_fat_sector + fat_size;
	Buffer<uint8_t> fat_buffer(sector_size);
	uint32_t free_cluster = -1;

	log(INFO, "Base FAT sector: %d, Max FAT sector: %d", fat_sector, max_fat_sector);

	for(size_t i = fat_sector; i < max_fat_sector; i++) {
		uint32_t base_cluster_entry_for_fat = (i - first_fat_sector) * (sector_size/4);
		log(INFO, "Base cluster: %p", base_cluster_entry_for_fat);
		auto lba = partmanager.get_lba(partid, i);
		volatile hal::DiskJob job{fat_buffer, lba, 1, false};
		hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
		if(job.state == hal::DiskJob::ERROR) { delete[] fat_buffer; return -1; }
		for(size_t j = (ent_offset/4); j < (sector_size / 4); j++) {
			const uint32_t* entry = reinterpret_cast<uint32_t*>(fat_buffer + 4*j);
			const auto cluster = base_cluster_entry_for_fat + j;
			if(cluster == search_from) continue;
			if(*entry == 0) {
				free_cluster = cluster;
				goto finish;
			}
		}
		ent_offset = 0;
	}
	finish:
	log(INFO, "Found free cluster: %d", free_cluster);

	return free_cluster;
}

bool FAT32::update_fsinfo(int diffclusters) {
	auto lba = partmanager.get_lba(partid, fsinfo_sector);
	Buffer<uint8_t> buffer(sector_size);
	volatile hal::DiskJob job{buffer, lba, 1, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) { return false; }

	uint32_t* sectorscount = reinterpret_cast<uint32_t*>(buffer + 0x1e8);
	*sectorscount += diffclusters;

	job.write = true;
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) { return false;}

	return true;
}

uint32_t FAT32::get_lookup_cluster(uint8_t* buffer) {
	auto lba = partmanager.get_lba(partid, fsinfo_sector);
	
	volatile hal::DiskJob job{buffer, lba, 1, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) { return false;}
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	uint32_t look_from = 2;
	if(*look_from_fsinfo != 0xFFFFFFFF && *look_from_fsinfo < (n_data_sectors / fat_boot->sectors_per_cluster)) {
		look_from = *look_from_fsinfo;
	}
	return look_from;
}

bool FAT32::save_lookup_cluster(uint32_t cluster, uint8_t* buffer) {
	auto lba = partmanager.get_lba(partid, fsinfo_sector);
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	*look_from_fsinfo = cluster;
	volatile hal::DiskJob job{buffer, lba, 1, true};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) {return false;}
	return true;
}

bool FAT32::alloc_clusters(uint32_t prevcluster, uint32_t nclusters) {
	// First thing to do: look at FSInfo so we know which cluster we need to start
	// searching from
	Buffer<uint8_t> buffer(sector_size);
	auto look_from = get_lookup_cluster(buffer);
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	Vector<uint32_t> clustervec;
	for(size_t i = 0; i < nclusters; i++) {
		auto free_cluster = search_free_cluster(look_from);
		log(INFO, "Next free cluster from %p: %p", look_from, free_cluster);
		look_from = free_cluster;
		clustervec.push_back(free_cluster);
	}

	set_next_cluster(prevcluster, clustervec[0]);
	for(int i = 0; i < (clustervec.size() - 1); i++) {
		set_next_cluster(clustervec[i], clustervec[i+1]);
		log(INFO, "Set link %p -> %p", clustervec[i], clustervec[i+1]);
	}

	set_next_cluster(clustervec.back(), FAT_FINISH);

	auto next_free = search_free_cluster(look_from);
	save_lookup_cluster(next_free, buffer);

	return true;
}

bool FAT32::save_cluster(uint32_t cluster, uint8_t* buffer) {
	auto sector = get_sector_of_cluster(cluster);
	auto lba = partmanager.get_lba(partid, sector);
	volatile hal::DiskJob job{buffer, lba, fat_boot->sectors_per_cluster, true};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);

	return job.state == hal::DiskJob::FINISHED;
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
	log(INFO, "Filename: %s", filename);
	const char* basename = rfind(filename, '/');
	if(!basename) return -1;
	uint32_t dir_cluster = get_parent_dir_cluster(filename, basename);
	basename++;
	if(dir_cluster >= FAT_ERROR) return dir_cluster;

	Buffer<uint8_t> buffer(cluster_size);
	do {
		log(INFO, "Loading cluster %d", dir_cluster);
		load_cluster(dir_cluster, buffer);
		uint32_t cluster;
		if((cluster = match_cluster(buffer, basename, FAT_FLAGS::ARCHIVE, nullptr)) != -1) {
			uint32_t cluster_offset = offset/cluster_size;
			for(size_t i = 0; i <= cluster_offset; i++) {
				dir_cluster = cluster;
				if(i < cluster_offset)
					cluster = next_cluster(cluster);
			}
			break;
		}
		dir_cluster = next_cluster(dir_cluster);
	} while(dir_cluster < FAT_ERROR);
	return dir_cluster;
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
	
	auto lba = partmanager.get_lba(partid, fat_sector);
	volatile hal::DiskJob job{fat_buffer, lba, 1, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::FINISHED) {
		uint32_t* table_value = (uint32_t*)&fat_buffer[ent_offset];

		*table_value = (next_cluster & 0x0FFFFFFF) | (*table_value & 0xF0000000);

		volatile hal::DiskJob writejob{fat_buffer, lba, 1, true};
		hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &writejob);
		if(job.state == hal::DiskJob::ERROR){ delete[] fat_buffer; return false;};

		// Update second fat table
		writejob.lba = lba + fat_size;
		hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &writejob);
		return writejob.state == hal::DiskJob::FINISHED;
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
		do {
			clusters.push_back(filecluster);
			filecluster = next_cluster(filecluster);
		} while(filecluster < FAT_ERROR);
		// Mark each and every cluster of the file as empty
		for(size_t i = 0; i < clusters.size(); i++) {
			set_next_cluster(clusters[i], 0);
		}
		zerosize = true;
		if(!update_fsinfo(clusters.size())) return 0;
	} else if(current_size_in_clusters > new_size_in_clusters) {
		log(INFO, "Current size in clusters (%d) less than new size in clusters (%d)", current_size_in_clusters, new_size_in_clusters);
		Vector<uint32_t> clusters;
		do {
			clusters.push_back(filecluster);
			filecluster = next_cluster(filecluster);
		} while(filecluster < FAT_ERROR);
		set_next_cluster(clusters[new_size_in_clusters - 1], FAT_FINISH);
		for(size_t i = current_size_in_clusters - new_size_in_clusters; i < clusters.size(); i++) {
			// Mark as free
			set_next_cluster(clusters[i], 0);
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
			auto last_filecluster = filecluster;
			do {
				last_filecluster = filecluster;
				filecluster = next_cluster(filecluster);
			} while(filecluster < FAT_ERROR);
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
	log(INFO, "Filename: %s", filename);
	const char* basename = rfind(filename, '/');
	if(!basename) return -1;
	auto dir_cluster = get_parent_dir_cluster(filename, basename);
	basename++;
	if(dir_cluster >= FAT_ERROR) return false;

	Buffer<uint8_t> buffer(cluster_size);
	do {
		log(INFO, "Loading cluster %d", dir_cluster);
		load_cluster(dir_cluster, buffer);
		uint32_t cluster;
		int nentry = -1;
		if((cluster = match_cluster(buffer, basename, FAT_FLAGS::ARCHIVE, nullptr, &nentry)) != -1) {
			dir_cluster = setstat(dir_cluster, nentry, properties) ? dir_cluster : FAT_ERROR;
			break;
		}
		dir_cluster = next_cluster(dir_cluster);
	} while(dir_cluster < FAT_ERROR);
	return dir_cluster < FAT_ERROR;
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
	uint8_t* ptr = buffer + 32*nentry;
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
	log(INFO, "Filename: %s", filename);
	const char* basename = rfind(filename, '/');
	if(!basename) return -1;
	auto dir_cluster = get_parent_dir_cluster(filename, basename);
	basename++;
	Buffer<uint8_t> buffer(cluster_size);
	do {
		log(INFO, "Loading cluster %d", dir_cluster);
		load_cluster(dir_cluster, buffer);
		uint32_t cluster;
		if((cluster = match_cluster(buffer, basename, FAT_FLAGS::ARCHIVE, buf)) != -1) {
			break;
		}
		dir_cluster = next_cluster(dir_cluster);
	} while(dir_cluster < FAT_ERROR);
	if(dir_cluster < FAT_ERROR)
		return 0;
	return -1;
}

uint32_t FAT32::match_cluster(uint8_t* buffer, const char* basename, FAT_FLAGS flags, struct stat* buf, int* nentry) {
	uint8_t lfn_order = 0;
	char lfn_buffer[256] = {0};
	bool has_lfn = false;
	const size_t entries_per_cluster = cluster_size / 32;
	for(size_t i = 0; i < entries_per_cluster; i++) {
		char name[14];
		memset(name, 0, sizeof(name));
		uint8_t* ptr_to_i = buffer + (32*i);
		FAT_FLAGS attr = static_cast<FAT_FLAGS>(ptr_to_i[11]);
		if(attr == FAT_FLAGS::LFN) {
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
			uint16_t low_16 = *reinterpret_cast<uint16_t*>(ptr_to_i + 26);
			uint16_t high_16 = *reinterpret_cast<uint16_t*>(ptr_to_i + 20);
			uint32_t cluster = (high_16 << 16) | low_16;

			char sfn_name[12] = {0};
			memcpy(sfn_name, ptr_to_i, 11);
			sfn_name[11] = 0;
			log(INFO, "%s %s %p", sfn_name, lfn_buffer, cluster);
			// If we match return
			if((static_cast<uint8_t>(attr) & static_cast<uint8_t>(flags)) && !filecmp(basename, has_lfn ? lfn_buffer : sfn_name, has_lfn)) {
				log(INFO, "looks like we have a match");
				if(buf) {
					direntrystat(ptr_to_i, buf, cluster);
				}
				if(nentry) {
					*nentry = i;
				}
				return cluster;
			}
			memset(lfn_buffer, 0, sizeof(lfn_buffer));
		} else {
			if(nentry && !strcmp(basename, "")) {
				*nentry = i;
				return i;
			}
		}
	}
	return -1;
}

void FAT32::direntrystat(uint8_t* direntry, struct stat *buf, uint32_t cluster) {
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

	buf->st_size = *reinterpret_cast<uint32_t*>(direntry + 28);
	buf->st_ino = cluster;
}

bool FAT32::touch(const char* filename) {
	struct stat buf;
	// Check whether file already exists
	int ret = stat(filename, &buf);

	if(ret == -1) {
		const char* basenameptr = rfind(filename, '/');
		if(!basenameptr) return false;
		uint32_t dir_cluster = get_parent_dir_cluster(filename, basenameptr);
		basenameptr++;

		if(dir_cluster < FAT_ERROR) {
			Buffer<uint8_t> buffer(cluster_size);
			int nentry = -1;
			do {
				load_cluster(dir_cluster, buffer);
				if(match_cluster(buffer, "", FAT_FLAGS::NONE, nullptr, &nentry) != -1)
					break;
				dir_cluster = next_cluster(dir_cluster);
			} while(dir_cluster < FAT_ERROR);
			/// Check if there is going to be enough size.
			char basename[256];
			size_t basenamelen = strlen(basenameptr);
			if(basenamelen > 255) return false;
			strncpy(basename, basenameptr, basenamelen);
			basename[basenamelen] = 0;
			// Round up (+12) + another required entry (non-LFN entry)
			const size_t nreqentries = ((basenamelen + 12 +13) / 13);
			const size_t ndirentries = cluster_size / 32;
			const size_t availentries = ndirentries - nentry;
			if(nreqentries > availentries) {
				// Allocate needed clusters + change nentry to point to the first
				// position since IIRC entries can't be spread across clusters
				auto required_clusters = (nreqentries - availentries + ndirentries - 1) / ndirentries;
				if(!alloc_clusters(dir_cluster, required_clusters)) return false;
				nentry = 0;
			}
			/// We have at least one entry in the directory
			if(nentry != -1) {
				for(size_t i = nentry; i < nentry + nreqentries - 1; i++) {
					uint8_t* ptr = buffer + 32*i;
					const int order = (i - nentry) + 1;
					const char* basenameptr = basename + ((order - 1) * 13);
					set_lfn_entry_data(ptr, basenameptr, order, i == nentry + nreqentries - 2);
				}
				// Finally the non-LFN entry
				uint8_t* ptr = buffer + 32*(nentry + nreqentries - 1);
				struct stat properties {
					0,
					TimeManager::get().get_time(),
					TimeManager::get().get_time(),
					TimeManager::get().get_time(),
					0
				};
				set_sfn_entry_data(ptr, basename, FAT_FLAGS::ARCHIVE, &properties);
				save_cluster(dir_cluster, buffer);
			}
			if(dir_cluster >= FAT_ERROR) return false;
			return true;
		}
	}
	return false;
}

void FAT32::set_lfn_entry_data(uint8_t* ptr, const char* basename, uint8_t order, bool is_last) {
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
	// If last entry mark it as such
	if(is_last) {
		ptr[0] |= 0x40;
	}
}

void FAT32::set_sfn_entry_data(uint8_t* ptr, const char* basename, FAT_FLAGS flags, const struct stat* properties) {
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
	if(basename) {
		char uppbasename[11];
		/// Default padding for short filenames are spaces
		memset(uppbasename, 0x20, sizeof(uppbasename));
		// Find the .
		const char* dot = rfind(basename, '.');
		if(dot) {
			// Copy the extension into the name
			memcpy(uppbasename + 8, dot + 1, 3);
			// Copy either first 8 chars or until the dot
			if(dot - basename > 8) {
				memcpy(uppbasename, basename, 8);
			} else {
				memcpy(uppbasename, basename, dot - basename);
			}
			
		} else {
			// Copy first 8 characters into name
			memcpy(uppbasename, basename, 8);
		}
		for(size_t i = 0; i < 11; i++) {
			uppbasename[i] = toupper(uppbasename[i]);
		}
		memcpy(ptr, uppbasename, sizeof(uppbasename));
	}
	if(flags != FAT_FLAGS::NONE) {
		ptr[OFF_ENTRY_ATTR] = static_cast<uint8_t>(flags);
	}
}