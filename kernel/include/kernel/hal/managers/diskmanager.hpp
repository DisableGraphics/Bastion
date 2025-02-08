#pragma once
#include <kernel/datastr/pair.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/hal/drvbase/disk.hpp>
#include <kernel/drivers/disk/diskname.hpp>
namespace hal {
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
			Disk* get_driver(size_t pos);
			/**
				\brief Get all disks and their names
			*/
			Vector<Pair<diskname, Disk*>>& get_disks();
			/**
				\brief Get number of disks
			*/
			size_t size() const;
			/**
				\brief Enqueues job but waits for the job to finish.
				\warning You must not call this function if you have a custom on_finish or on_error
				in the disk job. This function overwrites both.
				\warning You must invoke this function from a task, *after* the scheduler has
				started running, since this is implemented using a semaphore which needs the scheduler running.
			*/
			void sleep_job(size_t diskid, volatile DiskJob* job);
		private:
			DiskManager(){}
			~DiskManager();
			/// Initialise PCI-based disks
			void init_pci_disks();
			
			///	Vector that contains all disk controllers
			Vector<Pair<diskname, Disk*>> disk_controllers;
	};
}