#include <kernel/sync/spinlock.hpp>

void SpinLock::lock() {
	uint32_t expected = SPINLOCK_UNLOCK;
	while (true) {
		asm volatile (
			"lock xchg %0, %1"
			: "+r"(expected), "+m"(lock_var)
			:
			: "memory"
		);
		if (expected == SPINLOCK_UNLOCK)
			break;
		expected = SPINLOCK_UNLOCK;
	}
}

void SpinLock::unlock() {
	uint32_t dummy = SPINLOCK_UNLOCK;
	asm volatile (
		"xchg %0, %1"
		: "+r"(dummy), "+m"(lock_var)
		:
		: "memory"
	);
}