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

bool fs::BlockCache::read(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid) {
	log(INFO, "Reading into buffer %p with lba %p, size %p and disk #%d", buffer, lba, size, diskid);
	return disk_op(buffer, lba, size, diskid, readfn);
}

bool fs::BlockCache::write(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid) {
	log(INFO, "Writing from buffer %p with lba %p, size %p and disk #%d", buffer, lba, size, diskid);
	return disk_op(buffer, lba, size, diskid, writefn);
}

bool fs::BlockCache::disk_op(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid, void (*fn)(CacheBlock* cache, uint8_t* buffer, size_t sector_size)) {
	auto sector_size = hal::DiskManager::get().get_driver(diskid)->get_sector_size();
	size_t nsectors = size / sector_size;
	if(size % sector_size) nsectors++;
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
	
}