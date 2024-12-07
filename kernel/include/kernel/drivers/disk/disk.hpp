#pragma once
#include <kernel/datastr/vector.hpp>
#include <kernel/drivers/disk/disk_driver.hpp>

class DiskManager {
	public:
		static DiskManager &get();
		void init();
	private:
		DiskManager(){}
		Vector<DiskDriver> disk_controllers;
};