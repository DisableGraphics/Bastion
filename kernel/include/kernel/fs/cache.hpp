#pragma once
#include <stdint.h>
#include <kernel/datastr/bcache/bcache.hpp>
#include <kernel/datastr/buffer.hpp>
#include <kernel/datastr/uptr.hpp>

#define CAPACITY 512

struct CacheKey {
	size_t diskid;
	uint64_t lba;
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
			LRUCache<CacheKey, UniquePtr<Buffer<uint8_t>>> cache{CAPACITY};
	};
}