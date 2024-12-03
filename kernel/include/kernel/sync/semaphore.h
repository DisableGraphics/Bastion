#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* count; // Opaque pointer to internal semaphore data
} sem_t;
/**
	\brief Initialise the semaphore.
	\param initial_count Initial semaphore count.
 */
void semaphore_init(sem_t* sem, int initial_count);
/**
	\brief Destroy semaphore.
 */
void semaphore_destroy(sem_t* sem);
/**
	\brief Wait until semaphore is free.
 */
void semaphore_wait(sem_t* sem);
/**
	\brief Signal the semaphore that it is free.
 */
void semaphore_signal(sem_t* sem);

#ifdef __cplusplus
}
#endif