#include <kernel/sync/semaphore.h>
#include <stdatomic.h>
#include <stdlib.h>

// Internal structure (hidden from the user)
typedef struct {
	atomic_int count;
} InternalSemaphore;

void semaphore_init(sem_t* sem, int initial_count) {
	InternalSemaphore* internal = (InternalSemaphore*)kmalloc(sizeof(InternalSemaphore));
	if (!internal) {
		// Handle allocation failure
		abort();
	}
	atomic_init(&internal->count, initial_count);
	sem->count = internal;
}

void semaphore_destroy(sem_t* sem) {
	if (sem->count) {
		kfree(sem->count);
		sem->count = NULL;
	}
}

void semaphore_wait(sem_t* sem) {
	InternalSemaphore* internal = (InternalSemaphore*)sem->count;
	int expected;
	do {
		while ((expected = atomic_load(&internal->count)) <= 0) {
			__builtin_ia32_pause();
		}
	} while (!atomic_compare_exchange_weak(&internal->count, &expected, expected - 1));
}

void semaphore_signal(sem_t* sem) {
	InternalSemaphore* internal = (InternalSemaphore*)sem->count;
	atomic_fetch_add(&internal->count, 1);
}