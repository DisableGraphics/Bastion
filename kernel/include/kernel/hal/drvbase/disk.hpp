#pragma once
#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/hal/job/diskjob.hpp>
#include <kernel/datastr/vector.hpp>
namespace hal {
	/**
		\brief Base class of a disk driver.
		\details All disk drivers must implement this interface.
	*/
	class Disk : public Driver {
		public:
			/**
				\brief Destructor
			*/
			virtual ~Disk() = default;
			/**
				\brief Enqueues disk reading/writing job
				\return Whether the job has been enqueued correctly

				Doesn't check if the job has been *finished* correctly.
				To check that, check the status var in the job.
			*/
			virtual bool enqueue_job(volatile DiskJob* job) = 0;

			/** 
				\brief Get number of sectors of disk
			*/
			virtual uint64_t get_n_sectors() const = 0;
			/**
				\brief Get sector size of disk
				\return Sector size or default 512B
			*/
			virtual uint32_t get_sector_size() const { return 512; };
		protected:
			// Pending disk jobs
			Vector<DiskJob*> jobs;
	};
}