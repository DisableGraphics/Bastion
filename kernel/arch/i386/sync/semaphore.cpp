#include <kernel/sync/semaphore.hpp>

Semaphore::Semaphore(int count) {
	semaphore_init(&sem, count);
}

Semaphore::~Semaphore() {
	semaphore_destroy(&sem);
}

void Semaphore::await() {
	semaphore_wait(&sem);
}

void Semaphore::signal() {
	semaphore_signal(&sem);
}