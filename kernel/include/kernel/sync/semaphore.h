#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* count; // Opaque pointer to internal semaphore data
} sem_t;

void semaphore_init(sem_t* sem, int initial_count);
void semaphore_destroy(sem_t* sem);

void semaphore_wait(sem_t* sem);
void semaphore_signal(sem_t* sem);

#ifdef __cplusplus
}
#endif