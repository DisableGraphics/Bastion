#include <kernel/fs/cache.hpp>
#include <kernel/hal/managers/diskmanager.hpp>

fs::BlockCache& fs::BlockCache::get() {
	static fs::BlockCache instance;
	return instance;
}

bool fs::BlockCache::read(uint8_t* buffer, int64_t lba, size_t size, size_t diskid) {
	auto sector_size = hal::DiskManager::get().get_driver(diskid)->get_sector_size();
}