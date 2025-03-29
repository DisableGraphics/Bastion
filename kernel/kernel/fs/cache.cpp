#include <kernel/fs/cache.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/kernel/log.hpp>

fs::BlockCache& fs::BlockCache::get() {
	static fs::BlockCache instance;
	return instance;
}

static void readfn(CacheBlock* cache, uint8_t* buffer, size_t sector_size) {
	memcpy(buffer, cache->buffer, sector_size);
}

static void writefn(CacheBlock* cache, uint8_t* buffer, size_t sector_size) {
	memcpy(cache->buffer, buffer, sector_size);
	cache->dirty = true;
}

bool fs::BlockCache::read(uint8_t* buffer, uint64_t lba, size_t nsectors, size_t diskid) {
	log(INFO, "Reading into buffer %p with lba %p, size %p and disk #%d", buffer, lba, nsectors, diskid);
	mutx.lock();
	bool ret = disk_op(buffer, lba, nsectors, diskid, readfn);
	mutx.unlock();
	return ret;
}

bool fs::BlockCache::write(uint8_t* buffer, uint64_t lba, size_t nsectors, size_t diskid) {
	log(INFO, "Writing from buffer %p with lba %p, size %p and disk #%d", buffer, lba, nsectors, diskid);
	mutx.lock();
	bool ret = disk_op(buffer, lba, nsectors, diskid, writefn);
	mutx.unlock();
	return ret;
}

bool fs::BlockCache::disk_op(uint8_t* buffer, uint64_t lba, size_t nsectors, size_t diskid, void (*fn)(CacheBlock* cache, uint8_t* buffer, size_t sector_size)) {
	auto sector_size = hal::DiskManager::get().get_driver(diskid)->get_sector_size();
	for(size_t i = 0; i < nsectors; i++) {
		CacheKey key{diskid, lba + i};
		auto data = cache.get(key);
		if(!data) {
			cache.emplace(key, sector_size, lba, 1, diskid);
			data = cache.get(key);
			if(data) {
				volatile hal::DiskJob job{data->buffer, lba + i, 1, false};
				hal::DiskManager::get().sleep_job(diskid, &job);
			} else {
				// Massive error here. If we don't have an entry after emplacing
				// in the cache, we're absolutely fucked
				return false;
			}
		}
		fn(data, buffer + i * sector_size, sector_size);
	}
	return true;
}

bool fs::BlockCache::flush() {
	mutx.lock();
	for(CacheBlock& block : cache) {
		if(block.dirty) {
			log(INFO, "Saving sector %p", block.lba);
			///TODO: See why the fuck AHCI devices have a race condition in which I cannot input more than one command at a time
			volatile hal::DiskJob job{block.buffer, block.lba, block.size_in_sectors, true};
			hal::DiskManager::get().sleep_job(block.diskid, &job);
			if(job.state == hal::DiskJob::FINISHED) block.dirty = false;
		}
	}
	mutx.unlock();
	return true;
}