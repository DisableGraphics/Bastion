#include <kernel/kernel/panic.hpp>
#include <stdlib.h>

__attribute__((__noreturn__))
extern "C" void abort(void) {
#if defined(__is_libk)
	kn::panic("kernel: panic: abort()\n");
#else
	// TODO: Abnormally terminate the process as if by SIGABRT.
	printf("abort()\n");
#endif
	while (1) { }
	__builtin_unreachable();
}
