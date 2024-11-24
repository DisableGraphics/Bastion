#include <kernel/mutex.hpp>

void Mutex::lock() {
	uint32_t expected = MUTEX_UNLOCK;
	while (true) {
		asm volatile (
			"lock xchg %0, %1"
			: "+r"(expected), "+m"(lock_var)
			:
			: "memory"
		);
		if (expected == MUTEX_UNLOCK)
			break;
		expected = MUTEX_UNLOCK;
	}
}

void Mutex::unlock() {
	uint32_t dummy = MUTEX_UNLOCK;
	asm volatile (
		"xchg %0, %1"
		: "+r"(dummy), "+m"(lock_var)
		:
		: "memory"
	);
}