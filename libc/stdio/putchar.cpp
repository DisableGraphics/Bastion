#include <stdio.h>

#if defined(__is_libk)
#include <kernel/drivers/tty/ttyman.hpp>
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	TTYManager::get().write(&c, sizeof(c));
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}
