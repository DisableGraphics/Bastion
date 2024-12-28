#include <kernel/drivers/disk/job.hpp>

DiskJob::DiskJob(uint8_t* buffer, uint64_t lba, uint32_t sector_count, bool write) {
	this->buffer = buffer;
	this->lba = lba;
	this->sector_count = sector_count;
	this->write = write;
	this->state = WAITING;
}