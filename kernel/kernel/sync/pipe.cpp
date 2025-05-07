#include <kernel/sync/pipe.hpp>
#include <kernel/cpp/min.hpp>

Pipe::Pipe() {}

size_t Pipe::read(char* data, size_t size) {
	size_t bytes_read = 0;
	
	while (bytes_read < size) {
		// Wait for data to become available
		readable.acquire();
		lock.lock();
		
		// Calculate how much we can read
		size_t to_read = min(size - bytes_read, available_data());
		if (to_read == 0) {
			lock.unlock();
			break;
		}
		
		// Read data (handling circular buffer)
		size_t start = tail % PIPE_BUF_SIZE;
		size_t chunk = min(to_read, PIPE_BUF_SIZE - start);
		
		memcpy(data + bytes_read, buffer + start, chunk);
		if (chunk < to_read) {
			memcpy(data + bytes_read + chunk, buffer, to_read - chunk);
		}
		
		tail += to_read;
		bytes_read += to_read;
		
		lock.unlock();
		writable.release(); // Signal that space is available
	}
	
	return bytes_read;
}


size_t Pipe::write(const char* data, size_t size) {
	size_t written = 0;
	
	while (written < size) {
		// Wait for space to become available
		writable.acquire();
		lock.lock();
		
		// Calculate how much we can write
		size_t to_write = min(size - written, available_space());
		if (to_write == 0) {
			lock.unlock();
			break;
		}
		
		// Write data (handling circular buffer)
		size_t start = head % PIPE_BUF_SIZE;
		size_t chunk = min(to_write, PIPE_BUF_SIZE - start);
		
		memcpy(buffer + start, data + written, chunk);
		if (chunk < to_write) {
			memcpy(buffer, data + written + chunk, to_write - chunk);
		}
		
		head += to_write;
		written += to_write;
		
		lock.unlock();
		readable.release(); // Signal that data is available
	}
	
	return written;
}

size_t Pipe::available_space() const {
	if (head >= tail) {
		return PIPE_BUF_SIZE - (head - tail);
	}
	return tail - head - 1;
}

size_t Pipe::available_data() const {
	return head - tail;
}

