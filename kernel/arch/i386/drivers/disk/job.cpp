#include <kernel/drivers/disk/job.hpp>

DiskJob::DiskJob(uint8_t* buffer, uint64_t lba, uint32_t sector_count, bool write, 
	void (*on_finish)(volatile void*), volatile void* on_finish_args, 
	void (*on_error)(volatile void*), volatile void* on_error_args) {
	this->buffer = buffer;
	this->lba = lba;
	this->sector_count = sector_count;
	this->write = write;
	this->state = WAITING;

	this->on_finish = on_finish;
	this->on_error = on_error;

	this->on_finish_args = on_finish_args;
	this->on_error_args = on_error_args;
}