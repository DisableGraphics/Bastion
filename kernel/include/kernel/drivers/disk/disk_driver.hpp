#pragma once
#include <kernel/drivers/pci/pci.hpp>
#include <kernel/drivers/disk/job.hpp>
#include <kernel/datastr/vector.hpp>

class DiskDriver {
	public:
		DiskDriver(const PCI::PCIDevice &device);
		virtual ~DiskDriver() = default;
		/**
			\brief Enqueues disk reading/writing job
			\return Whether the job has been enqueued correctly

			Doesn't check if the job has been *finished* correctly.
			To check that, check the status var in the job.
		 */
		virtual bool enqueue_job(const DiskJob& job) = 0;

		// Get disk size in sectors
		virtual uint64_t get_disk_size() const = 0;
		virtual uint32_t get_sector_size() const { return 512; };
	protected:
		PCI::PCIDevice device;
		uint32_t bars[6];
		Vector<DiskJob> jobs;
};