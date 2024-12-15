#pragma once
#include <kernel/datastr/pair.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/uptr.hpp>
#include <kernel/drivers/disk/disk_driver.hpp>

struct diskname {
	diskname(const char *type, char number);
	operator char*();
	char type[4];
	char number;
	char zero = '\0';
};

class DiskManager {
	public:
		static DiskManager &get();
		void init();
	private:
		DiskManager(){}
		Vector<Pair<diskname, UniquePtr<DiskDriver>>> disk_controllers;
};