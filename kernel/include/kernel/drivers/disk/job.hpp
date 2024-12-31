#pragma once
#include <stdint.h>

/**
	\brief Disk job.

	Asks the disk to do a read or write operation.
 */

struct DiskJob {
	/**
		\brief Constructor.
		\param buffer Buffer to keep the data loaded from disk. Its size must be sector_count * sector_size.
		\param lba Logical Block Adressing of the sector to load on memory.
		\param sector_count Number of sectors to load. Max number depends on driver.
		\param write true -> Write, false -> Read
		\param on_finish What to do when the job has finished. Defaults to nullptr.
		\param on_finish_args Arguments to on_finish.
		\param on_error What to do when the job has had an error. Defaults to nullptr.
		\param on_error_args Arguments to on_error.
	 */
	DiskJob(uint8_t* buffer, uint64_t lba, uint32_t sector_count, bool write, 
	void (*on_finish)(volatile void*) = nullptr, volatile void* on_finish_args = nullptr, 
	void (*on_error)(volatile void*) = nullptr, volatile void* on_error_args = nullptr);

	/**
		\brief Current state of the job
	 */
	enum state {
		ERROR, // There has been an error while processing the job
		FINISHED, // The job has finished correctly
		WAITING // The job is waiting for completion
	} state;

	void (*on_finish)(volatile void*);
	void (*on_error)(volatile void*);

	volatile void *on_finish_args;
	volatile void *on_error_args;

	uint8_t * buffer;
	uint64_t lba;
	uint32_t sector_count;
	bool write;

	/**
		\brief Finishes job.

		Sets the state to FINISHED
		and executes on_finish if on_finish isn't nullptr
	 */
	void finish() volatile {
		state = FINISHED;
		if(on_finish) {
			on_finish(on_finish_args);
		}
	}
	/**
		\brief Error action.

		Sets the state to ERROR
		and executes on_error if on_error isn't nullptr
	 */
	void error() volatile {
		state = ERROR;
		if(on_error) {
			on_error(on_error_args);
		}
	}
};