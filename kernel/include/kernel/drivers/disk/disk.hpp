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

/**
	\brief Disk manager.
	Implemented as a singleton.
	Manages disk drivers.
	Can enqueue disk jobs.
 */
class DiskManager {
	public:
		/**
			\brief Get singleton instance.
		 */
		static DiskManager &get();
		/**
			\brief Initialise disk drivers.
		 */
		void init();

		/**
			\brief Enqueue disk job. Asynchronous.
			\param diskid The disk to run the job
			\param job The job to be ran by the disk

			\return Whether the job has been *enqueued* correctly.
			To know whether the job has *finished* correctly, check the
			status var in the job.
		 */
		bool enqueue_job(size_t diskid, volatile DiskJob* job);
		/**
			\brief Get a pointer to the driver in position pos
			\param pos Position of the driver in the internal vector
		 */
		DiskDriver* get_driver(size_t pos);
		/**
			\brief Get all disks and their names
		 */
		Vector<Pair<diskname, DiskDriver*>>& get_disks();
		/**
			\brief Get number of disks
		 */
		size_t size() const;
		/**
			\brief Enqueues job but waits for the job to finish using a spinlock.
		 */
		void spin_job(size_t diskid, volatile DiskJob* job);
	private:
		DiskManager(){}
		~DiskManager();
		/**
			Vector that contains all disk controllers
		 */
		Vector<Pair<diskname, DiskDriver*>> disk_controllers;
		// Number of disk controllers
};