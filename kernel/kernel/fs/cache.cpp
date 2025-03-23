#include <kernel/fs/cache.hpp>
#include <kernel/hal/managers/diskmanager.hpp>

fs::BlockCache& fs::BlockCache::get() {
	static fs::BlockCache instance;
	return instance;
}

bool fs::BlockCache::read(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid) {
	auto sector_size = hal::DiskManager::get().get_driver(diskid)->get_sector_size();
	size_t nsectors = size / sector_size;
	if(size % sector_size) nsectors++;
	for(size_t i = 0; i < nsectors; i++) {
		CacheKey key{diskid, lba + i};
		auto data = cache.get(key);
		if(!data) {
			auto unq = UniquePtr<Buffer<uint8_t>>(new Buffer<uint8_t>(sector_size));
			volatile hal::DiskJob job{*unq, lba + i, 1, false};
			hal::DiskManager::get().sleep_job(diskid, &job);
			cache.put(key, move(unq));
			data = cache.get(key);
		}
		memcpy(buffer + i * sector_size, data, sector_size);
	}
	return true;
}