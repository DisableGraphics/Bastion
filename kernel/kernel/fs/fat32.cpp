#include "kernel/time/time.hpp"
#include <stdint.h>
#include <kernel/fs/fat32.hpp>
#include <kernel/sync/semaphore.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <stdio.h>
#include <string.h>
#include <kernel/kernel/log.hpp>

#define FAT_ERROR 0x0FFFFFF7
#define FAT_FINISH 0x0FFFFFF8

FAT32::FAT32(PartitionManager &partmanager, size_t partid) : partmanager(partmanager) {
	this->partid = partid;
	sector_size = hal::DiskManager::get().get_driver(partmanager.get_disk_id())->get_sector_size();
	auto lba = partmanager.get_lba(partid, 0);

	fat_boot_buffer = new uint8_t[sector_size];

	volatile hal::DiskJob job{fat_boot_buffer, lba, 1, 0};
	hal::DiskManager::get().sleep_job(0, &job);
	fat_boot = reinterpret_cast<fat_BS_t*>(fat_boot_buffer);
	if(job.state == hal::DiskJob::FINISHED) {
		fat_extBS_32_t *fat_boot_ext_32 = reinterpret_cast<fat_extBS_32_t*>(&fat_boot->extended_section);
		total_sectors = (fat_boot->total_sectors_16 == 0)? fat_boot->total_sectors_32 : fat_boot->total_sectors_16;
		fat_size = (fat_boot->table_size_16 == 0)? fat_boot_ext_32->table_size_32 : fat_boot->table_size_16;
		memcpy(partname, fat_boot_ext_32->volume_label, 11);
		partname[11] = 0;
		
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
	delete[] fat_boot_buffer;
}

uint32_t FAT32::next_cluster(uint32_t active_cluster) {
	unsigned int fat_offset = active_cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	uint8_t fat_buffer[sector_size];
	
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

uint32_t FAT32::first_cluster_for_directory(const char* dir_path) {
	log(INFO, "FAT32::first_cluster_for_directory(%s)", dir_path);
	int ndirs;
	char** directories = substr(dir_path, '/', &ndirs);
	if(!directories) return -1;
	uint32_t dir_cluster = root_cluster;
	uint8_t * buffer = new uint8_t[sector_size];
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
	delete[] buffer;
	kfree(directories);
	return dir_cluster;
}

uint32_t FAT32::cluster_for_filename(const char* filename, unsigned offset) {
	log(INFO, "Filename: %s", filename);
	const char* basename = rfind(filename, '/');
	if(!basename) return -1;
	char * dir_path = new char[basename - filename+1];
	memcpy(dir_path, filename, basename - filename);
	dir_path[basename - filename] = 0;
	log(INFO, "Dir path: %s", dir_path);
	basename++;
	log(INFO, "Basename: %s", basename);

	uint32_t dir_cluster = first_cluster_for_directory(dir_path);
	delete[] dir_path;
	if(dir_cluster >= FAT_ERROR) return dir_cluster;

	uint8_t* buffer = new uint8_t[cluster_size];
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
	delete[] buffer;
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
		uint8_t *diskbuf = new uint8_t[cluster_size];
		for(size_t i = 0; cluster < FAT_ERROR && i < clusters; i++, cluster = next_cluster(cluster)) {
			load_cluster(cluster, diskbuf);
			memcpy(buffer, diskbuf+incluster_offset, incluster_nbytes);
			buffer += incluster_nbytes;
			nbytes -= incluster_nbytes;
			readbytes += incluster_nbytes;
			incluster_offset = 0;
			incluster_nbytes = nbytes > cluster_size ? cluster_size : nbytes;
		}
		delete[] diskbuf;

		return readbytes;
	}
	return -1;
}

off_t FAT32::truncate(const char* filename, unsigned nbytes) {
	auto filecluster = cluster_for_filename(filename, 0);
	struct stat buf;
	int ret = stat(filename, &buf);
	if(ret == -1) return -1;
	auto current_size_in_clusters = (buf.st_size + sector_size - 1) / cluster_size;
	auto new_size_in_clusters = (nbytes + sector_size -1) / cluster_size;

	if(current_size_in_clusters > new_size_in_clusters) {

	} else if (current_size_in_clusters < new_size_in_clusters) {

	} else { // Same cluster size

	}
}

int FAT32::stat(const char* filename, struct stat* buf) {
	log(INFO, "Filename: %s", filename);
	const char* basename = rfind(filename, '/');
	if(!basename) return -1;
	char * dir_path = new char[basename - filename+1];
	memcpy(dir_path, filename, basename - filename);
	dir_path[basename - filename] = 0;
	log(INFO, "Dir path: %s", dir_path);
	basename++;
	log(INFO, "Basename: %s", basename);

	uint32_t dir_cluster = first_cluster_for_directory(dir_path);
	delete[] dir_path;
	if(dir_cluster >= FAT_ERROR) return dir_cluster;

	uint8_t* buffer = new uint8_t[cluster_size];
	do {
		log(INFO, "Loading cluster %d", dir_cluster);
		load_cluster(dir_cluster, buffer);
		uint32_t cluster;
		if((cluster = match_cluster(buffer, basename, FAT_FLAGS::ARCHIVE, buf)) != -1) {
			break;
		}
		dir_cluster = next_cluster(dir_cluster);
	} while(dir_cluster < FAT_ERROR);
	delete[] buffer;
	if(dir_cluster < FAT_ERROR)
		return 0;
	return -1;
}

uint32_t FAT32::match_cluster(uint8_t* buffer, const char* basename, FAT_FLAGS flags, struct stat* buf) {
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
				if(buf != nullptr) {
					direntrystat(ptr_to_i, buf);
				}
				return cluster;
			}
			memset(lfn_buffer, 0, sizeof(lfn_buffer));
		}
	}
	return -1;
}

void FAT32::direntrystat(uint8_t* direntry, struct stat *buf) {
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
	buf->st_mtime = buf->st_ctime;

	log(INFO, "%d-%d-%d %d:%d:%d", creatd.year, creatd.month, creatd.day, creatd.hour, creatd.minute, creatd.second);

	uint16_t acctime = *reinterpret_cast<uint16_t*>(direntry + 22);
	// Format: 0000 0|000 000|0 0000
	date accdt;
	accdt.second = (creattime & 0x1f) * 2;
	accdt.minute = (creattime & 0x7E0) >> 5;
	accdt.hour = (creattime & 0xF800) >> 11;

	// Format: 0000 000|0 000|0 0000
	uint16_t accdate = *reinterpret_cast<uint16_t*>(direntry + 18);
	accdt.day = (accdate & 0x1f);
	accdt.month = (accdate & 0x1E0) >> 5;
	accdt.year = ((accdate & 0xFE00) >> 9) + 1980;
	buf->st_atime = TimeManager::date_to_timestamp(accdt);

	buf->st_size = *reinterpret_cast<uint32_t*>(direntry + 28);
}