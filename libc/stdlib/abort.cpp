#include <kernel/kernel/panic.hpp>
#include <stdlib.h>


extern "C" [[gnu::noreturn]] void abort(void) {
	kn::panic("kernel: panic: abort()\n");
	while (1) { }
	__builtin_unreachable();
}
