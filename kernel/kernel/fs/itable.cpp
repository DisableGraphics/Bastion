#include <kernel/fs/itable.hpp>

ITable& ITable::get() {
	static ITable instance;
	return instance;
}

int ITable::add_entry(enum ITableEntry::fs_type type, uint32_t partition_id, void *fs_internal) {
	ITableEntry *entry = new ITableEntry(type, partition_id, fs_internal);
	table.push_back(entry);
	return table.size() - 1;
}

int ITable::remove_entry(int id) {
    if (id < 0 || id >= static_cast<int>(table.size())) {
        return -1;
    }

    ITableEntry *entry = table[id];
    if (entry->refcount > 1) {
        entry->refcount--;
        return 0;
    }

    delete entry;
    table[id] = nullptr;
    return 0;
}

// Get an entry by ID
ITableEntry* ITable::get_entry(int id) {
    if (id < 0 || id >= static_cast<int>(table.size())) {
        return nullptr;
    }
    return table[id];
}