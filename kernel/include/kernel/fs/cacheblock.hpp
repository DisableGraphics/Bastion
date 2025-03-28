#pragma once
#include <kernel/datastr/buffer.hpp>
#include <stdint.h>
#include <kernel/hal/job/diskjob.hpp>
#include <kernel/hal/managers/diskmanager.hpp>

struct CacheBlock {
	uint64_t lba;
	size_t size_in_sectors;
	size_t diskid;
	Buffer<uint8_t> buffer;
	bool dirty = false;
	CacheBlock(size_t size, uint64_t lba, size_t size_in_sectors, size_t diskid) : 
		buffer(size),
		lba(lba),
		size_in_sectors(size_in_sectors),
		diskid(diskid)
		{}
	~CacheBlock() {
		// Save the block to disk
		volatile hal::DiskJob job{
			buffer,
			lba,
			size_in_sectors,
			true
		};
		hal::DiskManager::get().sleep_job(diskid, &job);
	}
};