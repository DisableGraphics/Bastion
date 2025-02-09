#include <kernel/fs/fat32.hpp>
#include <kernel/sync/semaphore.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <stdio.h>
#include <string.h>
#include <kernel/kernel/log.hpp>

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
		log(INFO,"Total sectors: %d. FAT Size: %d. Cluster size: %d. Name: %s", total_sectors, fat_size, fat_boot->sectors_per_cluster, partname);
		log(INFO,"Total clusters: %d. First FAT sector: %d. Number of data sectors: %d. Root cluster: %d", total_clusters, first_fat_sector, n_data_sectors, root_cluster);
	} else {
		log(ERROR, "Could not load FAT from lba %d", lba);
		return;
	}
	if(load_fat_sector(root_cluster)) {
		log(INFO,"Loaded root cluster from fat correctly");
	} else {
		log(INFO,"Could not load root cluster from fat");
	}
}

FAT32::~FAT32() {
	delete[] fat_boot_buffer;
}

bool FAT32::load_fat_sector(uint32_t active_cluster) {
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
		if(table_value >= 0x0FFFFFF8) {
			log(INFO,"Chain finished");
		} else if(table_value == 0x0FFFFFF7) {
			log(INFO,"Bad sector");
		} else {
			log(INFO,"Next cluster: %p", table_value);
		}
		return true;
	}
	return false;
}

uint32_t FAT32::get_sector_of_cluster(uint32_t cluster) {
	return ((cluster - 2) * fat_boot->sectors_per_cluster) + first_data_sector;
}

