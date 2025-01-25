#include <kernel/sync/semaphore.hpp>

SpinSemaphore::SpinSemaphore(int count) {
	semaphore_init(&sem, count);
}

SpinSemaphore::~SpinSemaphore() {
	semaphore_destroy(&sem);
}

void SpinSemaphore::await() {
	semaphore_wait(&sem);
}

void SpinSemaphore::signal() {
	semaphore_signal(&sem);
}