#pragma once
#include <kernel/sync/mutex.hpp>
#include <kernel/sync/semaphore.hpp>

class Pipe {
	public:
		Pipe();
		size_t write(const char* data, size_t size);
		size_t read(char* data, size_t size);
	private:
		static const int PIPE_BUF_SIZE = 4096;	
		char buffer[PIPE_BUF_SIZE];
		size_t head = 0;        // Write position
		size_t tail = 0;        // Read position
		Mutex lock;      // Mutual exclusion
		Semaphore readable{0, PIPE_BUF_SIZE};  // Counts readable bytes
		Semaphore writable{PIPE_BUF_SIZE, PIPE_BUF_SIZE}; // Counts writable space
		size_t available_space() const;
		size_t available_data() const;
};