#pragma once
#include <kernel/datastr/pair.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/drivers/disk/job.hpp>
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

		bool enqueue_job(size_t diskid, volatile DiskJob* job);
		
		DiskDriver* get_driver(size_t pos);
		Vector<Pair<diskname, DiskDriver*>>& get_disks();
		size_t size() const;
	private:
		DiskManager(){}
		~DiskManager();
		Vector<Pair<diskname, DiskDriver*>> disk_controllers;
		size_t ndevices;
};