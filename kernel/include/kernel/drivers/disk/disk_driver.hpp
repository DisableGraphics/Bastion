#pragma once
#include <kernel/drivers/pci/pci.hpp>
#include <kernel/drivers/disk/job.hpp>
#include <kernel/datastr/vector.hpp>

class DiskDriver {
	public:
		/**
			\brief Disk driver constructor
		 */
		DiskDriver(const PCI::PCIDevice &device);
		/**
			\brief Destructor
		 */
		virtual ~DiskDriver() = default;
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
		// PCI device and info
		PCI::PCIDevice device;
		uint32_t bars[6];
		Vector<DiskJob*> jobs;
};