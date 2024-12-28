#include <kernel/fs/partmanager.hpp>
#include <kernel/drivers/disk/job.hpp>
#include <kernel/drivers/disk/disk.hpp>

#include <kernel/kernel/panic.hpp>

PartitionManager::PartitionManager(size_t disk_id) {
	this->disk_id = disk_id;

	uint8_t *buffer = new uint8_t[512];
	volatile DiskJob *job = new DiskJob(buffer, 0, 1, 0);
	DiskManager::get().enqueue_job(0, job);
	while(job->state == DiskJob::WAITING)
		;
	if(job->state == DiskJob::ERROR) {
		kn::panic("Error while loading partition table from disk\n");
	}
	for(size_t i = 0; i < 4; i++) {
		Partition * part = reinterpret_cast<Partition*>(job->buffer + (0x01BE + sizeof(Partition)*i));
		if(part->total_sectors != 0) {
			Partition p = *part;
			parts.push_back(p);
		}
	}

	delete[] buffer;
	delete job;
}

Vector<Partition> PartitionManager::get_partitions() {
	return parts;
}

uint64_t PartitionManager::get_lba(size_t partition_id, uint64_t sector) {
	return parts[partition_id].lba + sector;
}

size_t PartitionManager::get_disk_id() {
	return disk_id;
}