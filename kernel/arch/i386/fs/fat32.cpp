#include <kernel/fs/fat32.hpp>
#include <kernel/drivers/disk/disk.hpp>
#include <stdio.h>
#include <string.h>

FAT32::FAT32(PartitionManager &partmanager, size_t partid) : partmanager(partmanager) {
	this->partid = partid;

	sector_size = DiskManager::get().get_driver(partmanager.get_disk_id())->get_sector_size();
	
	auto lba = partmanager.get_lba(partid, 0);
	volatile DiskJob job = DiskJob(fat_boot_buffer, lba, 1, false);
	DiskManager::get().spin_job(partmanager.get_disk_id(), &job);
	fat_boot = reinterpret_cast<fat_BS_t*>(fat_boot_buffer);
	if(job.state == DiskJob::FINISHED) {
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
		printf("Total sectors: %d. FAT Size: %d. Cluster size: %d. Name: %s\n", total_sectors, fat_size, fat_boot->sectors_per_cluster, partname);
		printf("Total clusters: %d. First FAT sector: %d. Number of data sectors: %d. Root cluster: %d\n", total_clusters, first_fat_sector, n_data_sectors, root_cluster);
	}
	if(load_fat_sector(2)) {
		printf("Loaded sector 2 from fat correctly\n");
	} else {
		printf("Could not load sector 2 from fat\n");
	}
}

FAT32::~FAT32() {

}

bool FAT32::load_fat_sector(uint32_t active_cluster) {
	unsigned int fat_offset = active_cluster * 4;
	unsigned int fat_sector = first_fat_sector + (fat_offset / sector_size);
	unsigned int ent_offset = fat_offset % sector_size;
	uint8_t fat_buffer[sector_size];
	auto lba = partmanager.get_lba(partid, fat_sector);
	volatile DiskJob job{fat_buffer, lba, 1, false};
	DiskManager::get().spin_job(partmanager.get_disk_id(), &job);
	if(job.state == DiskJob::FINISHED) {
		uint32_t table_value = *(uint32_t*)&fat_buffer[ent_offset];
		table_value &= 0x0FFFFFFF;
		if(table_value >= 0x0FFFFFF8) {
			printf("Chain finished\n");
		} else if(table_value == 0x0FFFFFF7) {
			printf("Bad sector\n");
		} else {
			printf("Next cluster: %p\n", table_value);
		}
		return true;
	}
	return false;
}

uint32_t FAT32::get_sector_of_cluster(uint32_t cluster) {
	return ((cluster - 2) * fat_boot->sectors_per_cluster) + first_data_sector;
}

