#include <kernel/kernel/panic.hpp>
#include <kernel/sync/spinlock.hpp>

extern "C" void __cxa_pure_virtual() {
	kn::panic("__cxa_pure_virtual() called");
}

namespace __cxxabiv1 
{
	SpinLock lc;
	/* guard variables */

	/* The ABI requires a 64-bit type.  */
	__extension__ typedef int __guard __attribute__((mode(__DI__)));

	extern "C" int __cxa_guard_acquire(__guard *);
	extern "C" void __cxa_guard_release(__guard *);
	extern "C" void __cxa_guard_abort(__guard *);

	extern "C" int __cxa_guard_acquire(__guard *g) {
		lc.lock();

		// Check if initialized (guard first byte == 1)
		if (*(char *)g == 1) {
			lc.unlock();
			return 0;  // already initialized, no need to construct
		}
		// Not initialized yet, keep lock held to serialize init
		return 1;

	}

	extern "C" void __cxa_guard_release(__guard *g) {
		*(char *)g = 1;  // mark initialized
    	lc.unlock();
	}

	extern "C" void __cxa_guard_abort(__guard *) {
		lc.unlock();
	}
}
