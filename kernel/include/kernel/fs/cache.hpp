#pragma once
#include <stdint.h>
#include <kernel/datastr/lrucache/lrucache.hpp>
#include <kernel/datastr/buffer.hpp>
#include <kernel/datastr/uptr.hpp>
#include <kernel/datastr/hashmap/hash.hpp>

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
		return key.diskid ^ key.lba;
	}
};

namespace fs {
	class BlockCache {
		public:
			static BlockCache& get();
			bool read(uint8_t* buffer, uint64_t lba, size_t size, size_t diskid);

		private:
			BlockCache(){};
			BlockCache(const BlockCache& other) = delete;
			BlockCache(BlockCache&& other) = delete;

			void operator=(const BlockCache& other) = delete;
			void operator=(BlockCache&& other) = delete;
			LRUCache<CacheKey, Buffer<uint8_t>> cache{CAPACITY};
	};
}