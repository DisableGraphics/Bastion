#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.hpp>
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	TTY::get().write(&c, sizeof(c));
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}