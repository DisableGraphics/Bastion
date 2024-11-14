#include <kernel/mutex.hpp>

void ExplicitMutex::acquire() {
	while(atomic_flag_test_and_set(&mutex))
	{
		__builtin_ia32_pause();
	}
}

void ExplicitMutex::release() {
	atomic_flag_clear(&mutex);
}

Mutex::Mutex() {
	mut.acquire();
}

Mutex::~Mutex() {
	mut.release();
}