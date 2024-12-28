#include <kernel/fs/fat32.hpp>
#include <kernel/drivers/disk/disk.hpp>
#include "../defs/fs/fat32/bs.hpp"

#include <stdio.h>
#include <string.h>

FAT32::FAT32(PartitionManager &partmanager, size_t partid) {
	this->partid = partid;
	
	auto lba = partmanager.get_lba(partid, 0);
	uint8_t * buffer = new uint8_t[512];
	volatile DiskJob *job = new DiskJob(buffer, lba, 1, false);
	DiskManager::get().enqueue_job(partmanager.get_disk_id(), job);
	while(job->state == DiskJob::WAITING);
	if(job->state == DiskJob::FINISHED) {
		fat_BS_t* fat_boot = reinterpret_cast<fat_BS_t*>(job->buffer);
		fat_extBS_32_t *fat_boot_ext_32 = reinterpret_cast<fat_extBS_32_t*>(&fat_boot->extended_section);
		total_sectors = (fat_boot->total_sectors_16 == 0)? fat_boot->total_sectors_32 : fat_boot->total_sectors_16;
		fat_size = (fat_boot->table_size_16 == 0)? fat_boot_ext_32->table_size_32 : fat_boot->table_size_16;
		memcpy(partname, fat_boot_ext_32->volume_label, 11);
		partname[11] = 0;
		printf("Total sectors: %d. FAT Size: %d. Name: %s\n", total_sectors, fat_size, partname);
	}

	delete[] buffer;
	delete job;
}

