#include "kernel/datastr/vector.hpp"
#include "kernel/hal/job/diskjob.hpp"
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
	delete[] fat_boot_buffer;
}

uint32_t FAT32::next_cluster(uint32_t active_cluster) {
	unsigned int fat_offset = active_cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	uint8_t* fat_buffer = new uint8_t[sector_size];
	
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
		delete[] fat_buffer;
		return table_value;
	}
	delete[] fat_buffer;
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
	uint8_t* fat_buffer = new uint8_t[sector_size];
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
	delete[] fat_buffer;

	return free_cluster;
}

bool FAT32::update_fsinfo(int diffclusters) {
	auto lba = partmanager.get_lba(partid, fsinfo_sector);
	uint8_t* buffer = new uint8_t[sector_size];
	volatile hal::DiskJob job{buffer, lba, 1, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) { delete[] buffer; return false; }

	uint32_t* sectorscount = reinterpret_cast<uint32_t*>(buffer + 0x1e8);
	*sectorscount += diffclusters;

	job.write = true;
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) { delete[] buffer; return false;}

	delete[] buffer;
	return true;
}

bool FAT32::alloc_clusters(uint32_t prevcluster, uint32_t nclusters) {
	// First thing to do: look at FSInfo so we know which cluster we need to start
	// searching from
	auto lba = partmanager.get_lba(partid, fsinfo_sector);
	uint8_t* buffer = new uint8_t[sector_size];
	volatile hal::DiskJob job{buffer, lba, 1, false};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &job);
	if(job.state == hal::DiskJob::ERROR) { delete[] buffer; return false;}
	uint32_t* look_from_fsinfo = reinterpret_cast<uint32_t*>(buffer + 0x1EC);
	uint32_t look_from = 2;
	if(*look_from_fsinfo != 0xFFFFFFFF && *look_from_fsinfo < (n_data_sectors / fat_boot->sectors_per_cluster)) {
		look_from = *look_from_fsinfo;
	}
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
	*look_from_fsinfo = next_free;
	volatile hal::DiskJob savejob{buffer, lba, 1, true};
	hal::DiskManager::get().sleep_job(partmanager.get_disk_id(), &savejob);
	if(savejob.state == hal::DiskJob::ERROR) { delete[] buffer; return false;}

	delete[] buffer;
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

bool FAT32::set_next_cluster(uint32_t cluster, uint32_t next_cluster) {
	unsigned int fat_offset = cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;

	uint8_t* fat_buffer = new uint8_t[sector_size];
	
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
		delete[] fat_buffer;
		return writejob.state == hal::DiskJob::FINISHED;
	}
	delete[] fat_buffer;
	return false;
}

off_t FAT32::truncate(const char* filename, unsigned nbytes) {
	auto filecluster = cluster_for_filename(filename, 0);
	struct stat buf;
	int ret = stat(filename, &buf);
	if(ret == -1) return -1;
	uint32_t current_size_in_clusters = (static_cast<uint32_t>(buf.st_size) + sector_size - 1) / cluster_size;
	auto new_size_in_clusters = (nbytes + sector_size -1) / cluster_size;

	if(current_size_in_clusters > new_size_in_clusters) {
		log(INFO, "Current size in clusters (%d) less than new size in clusters (%d)", current_size_in_clusters, new_size_in_clusters);
		Vector<uint32_t> clusters;
		do {
			clusters.push_back(filecluster);
			filecluster = next_cluster(filecluster);
		} while(filecluster < FAT_ERROR);
		uint32_t newlastsectorpos = current_size_in_clusters - new_size_in_clusters - 1;
		set_next_cluster(clusters[newlastsectorpos], FAT_FINISH);
		for(size_t i = current_size_in_clusters - new_size_in_clusters; i < clusters.size(); i++) {
			// Mark as free
			set_next_cluster(clusters[i], 0);
		}
		log(INFO, "Size change from %d to %d sectors", current_size_in_clusters, new_size_in_clusters);
		update_fsinfo(current_size_in_clusters - new_size_in_clusters);
	} else if (current_size_in_clusters < new_size_in_clusters) {
		auto last_filecluster = filecluster;
		do {
			last_filecluster = filecluster;
			filecluster = next_cluster(filecluster);
		} while(filecluster < FAT_ERROR);
		if(!alloc_clusters(last_filecluster, new_size_in_clusters - current_size_in_clusters)) return 0;
		update_fsinfo(current_size_in_clusters - new_size_in_clusters);
	}
	// Update the size of the file.
	struct stat properties{
		nbytes,
		TimeManager::get().get_time(),
		TimeManager::get().get_time(),
		-1
	};
	file_setproperty(filename, &properties);
	return nbytes;
}

bool FAT32::file_setproperty(const char* filename, const struct stat* properties) {
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
	if(dir_cluster >= FAT_ERROR) return false;

	uint8_t* buffer = new uint8_t[cluster_size];
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
	delete[] buffer;
	return dir_cluster < FAT_ERROR;
}

bool FAT32::setstat(uint32_t dir, int nentry, const struct stat* properties) {
	uint8_t* buffer = new uint8_t[cluster_size];
	load_cluster(dir, buffer);

	uint8_t* ptr = buffer + 32*nentry;

	if(properties->st_atime != -1) {
		date dt = TimeManager::timestamp_to_date(properties->st_atime);
		uint16_t* date = reinterpret_cast<uint16_t*>(ptr + 18);
		uint16_t datecalc = 0;
		datecalc |= (dt.year & 0x7F) << 9;
		datecalc |= (dt.month & 0xF) << 5;
		datecalc |= (dt.day & 0x1F);

		*date = datecalc;
	}
	if(properties->st_mtime != -1) {
		date dt = TimeManager::timestamp_to_date(properties->st_mtime);
		uint16_t* date = reinterpret_cast<uint16_t*>(ptr + 24);
		uint16_t datecalc = 0;
		datecalc |= (dt.year & 0x7F) << 9;
		datecalc |= (dt.month & 0xF) << 5;
		datecalc |= (dt.day & 0x1F);

		*date = datecalc;
		uint16_t* time = reinterpret_cast<uint16_t*>(ptr + 22);
		uint16_t timecalc = 0;
		timecalc |= (dt.year & 0x1F) << 11;
		timecalc |= (dt.month & 0x3F) << 5;
		timecalc |= ((dt.second/2) & 0x1F);

		*time = timecalc;
	}
	if(properties->st_ctime != -1) {
		date dt = TimeManager::timestamp_to_date(properties->st_mtime);
		uint16_t* date = reinterpret_cast<uint16_t*>(ptr + 16);
		uint16_t datecalc = 0;
		datecalc |= (dt.year & 0x7F) << 9;
		datecalc |= (dt.month & 0xF) << 5;
		datecalc |= (dt.day & 0x1F);

		*date = datecalc;
		uint16_t* time = reinterpret_cast<uint16_t*>(ptr + 14);
		uint16_t timecalc = 0;
		timecalc |= (dt.year & 0x1F) << 11;
		timecalc |= (dt.month & 0x3F) << 5;
		timecalc |= ((dt.second/2) & 0x1F);

		*time = timecalc;
	}
	if(properties->st_size != -1) {
		uint32_t* sizeptr = reinterpret_cast<uint32_t*>(ptr + 28);
		*sizeptr = properties->st_size;
	}

	auto ret = save_cluster(dir, buffer);
	delete[] buffer;
	return ret;
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
	if(dir_cluster >= FAT_ERROR) return -1;

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
					direntrystat(ptr_to_i, buf);
				}
				if(nentry) {
					*nentry = i;
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
}