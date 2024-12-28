#pragma once
#include <stddef.h>
#include <kernel/fs/part.hpp>
#include <kernel/datastr/vector.hpp>

class PartitionManager {
	public:
		PartitionManager(size_t disk_id);
		Vector<Partition> get_partitions();
		uint64_t get_lba(size_t partition_id, uint64_t sector);
		size_t get_disk_id();
	private:
		size_t disk_id;
		Vector<Partition> parts;
};