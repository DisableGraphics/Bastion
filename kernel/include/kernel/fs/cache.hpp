#pragma once
#include <stdint.h>
#include <kernel/datastr/lrucache/lrucache.hpp>
#include <kernel/datastr/buffer.hpp>
#include <kernel/datastr/uptr.hpp>
#include <kernel/datastr/hashmap/hash.hpp>
#include <kernel/fs/cacheblock.hpp>

#define CAPACITY 512

struct CacheKey {
	size_t diskid;
	uint64_t lba;

	bool operator==(const CacheKey& other) {
		return diskid == other.diskid && lba == other.lba;
	}
};

template<>
struct Hash<CacheKey>
{
	size_t operator()(const CacheKey& key) {
		// Cursed hash that kinda works
		return key.diskid ^ (key.lba + 0x9e3779b9 + (key.diskid << 6) + (key.diskid >> 2));
	}
};

namespace fs {
	class BlockCache {
		public:
			static BlockCache& get();
			/// Read from disk into a buffer
			bool read(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid);
			/// Write from a buffer into the cache.
			/// \warning This operation DOES NOT write into disk, only to the cache.
			/// Use flush() if you want to actually commit the changes to disk.
			bool write(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid);
			/// Commits all changes of non-evicted dirty sectors in the cache
			bool flush();
		private:
			bool disk_op(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid, void (*fn)(CacheBlock* cache, uint8_t* buffer, size_t sector_size));
			BlockCache(){};
			BlockCache(const BlockCache& other) = delete;
			BlockCache(BlockCache&& other) = delete;

			void operator=(const BlockCache& other) = delete;
			void operator=(BlockCache&& other) = delete;
			LRUCache<CacheKey, CacheBlock> cache{CAPACITY};
	};
}