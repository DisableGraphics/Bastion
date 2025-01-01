#include <kernel/datastr/vector.hpp>
#include <stdint.h>

struct ITableEntry {
	enum fs_type {
		FAT32
	} fs_type;
	uint32_t partition_id;
	void * fs_internal;
	uint64_t position;
	uint32_t flags;
	uint32_t refcount;
	ITableEntry(enum fs_type type, uint32_t part_id, void* internal)
        : fs_type(type), partition_id(part_id), fs_internal(internal), position(0), flags(0), refcount(1) {}

};

class ITable {
	public:
		static ITable& get();
		int add_entry(enum ITableEntry::fs_type type, uint32_t partition_id, void *fs_internal);
		int remove_entry(int id);
		ITableEntry* get_entry(int id);
	private:
		Vector<ITableEntry*> table;
		ITable(){};
};