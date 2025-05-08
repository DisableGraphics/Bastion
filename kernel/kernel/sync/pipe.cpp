#include <kernel/sync/pipe.hpp>
#include <kernel/cpp/min.hpp>

Pipe::Pipe() {}

size_t Pipe::read(char* data, size_t size) {
	lock.lock();
	if(count < size) {
		lock.unlock();
		return -1;
	}
	
	for (int i = 0; i < size; i++) {
        data[i] = buffer[tail];
        tail = (tail + 1) % PIPE_BUF_SIZE;
        count--;
        writable.release();
    }
	lock.unlock();
	return size;
}


size_t Pipe::write(const char* data, size_t size) {
	if(size > PIPE_BUF_SIZE){
		return -1;
	}
	lock.lock();
	if ((PIPE_BUF_SIZE - count) < size) {
        lock.unlock();
        return -1;
    }

	for (int i = 0; i < size; i++) {
        buffer[head] = data[i];
        head =  (head + 1) % PIPE_BUF_SIZE;
        count++;
        readable.release();
    }

	lock.unlock();
	return size;
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

