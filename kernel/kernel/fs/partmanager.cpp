#include <kernel/fs/partmanager.hpp>
#include <kernel/hal/job/diskjob.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/kernel/log.hpp>

#include <kernel/kernel/panic.hpp>
#include <string.h>

struct [[gnu::packed]] MBR_Partition {
		uint8_t boot_indicator;
		uint8_t starting_head;
		uint8_t starting_sector : 6;
		uint16_t starting_cylinder : 10;
		uint8_t system_id;
		uint8_t ending_head;
		uint8_t ending_sector: 6;
		uint16_t ending_cylinder: 10;
		uint32_t lba;
		uint32_t total_sectors;
};

PartitionManager::PartitionManager(size_t disk_id) {
	this->disk_id = disk_id;

	uint8_t *buffer = new uint8_t[hal::DiskManager::get().get_driver(disk_id)->get_sector_size()];
	memset(buffer, 0, sizeof(hal::DiskManager::get().get_driver(disk_id)->get_sector_size()));

	volatile hal::DiskJob job{buffer, 0, 1, 0};
	hal::DiskManager::get().sleep_job(0, &job);

	if(job.state == hal::DiskJob::ERROR) {
		kn::panic("Error while loading partition table from disk\n");
	}
	for(size_t i = 0; i < 4; i++) {
		MBR_Partition * part = reinterpret_cast<MBR_Partition*>(job.buffer + (0x01BE + sizeof(MBR_Partition)*i));
		if(part->total_sectors != 0) {
			log(INFO, "Partition %d in disk exists", i);
			Partition p = {i, part->lba, part->total_sectors, part->system_id};
			parts.push_back(p);
		}
	}
	delete[] buffer;
}

Vector<Partition> PartitionManager::get_partitions() {
	return parts;
}

uint64_t PartitionManager::get_lba(size_t partition_id, uint64_t sector) {
	return parts[partition_id].start_lba + sector;
}

size_t PartitionManager::get_disk_id() {
	return disk_id;
}