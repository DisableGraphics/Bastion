#include <kernel/panic.hpp>

void kn::panic(const char *str) {
	printf("Kernel panic: %s\n", str);
	halt();
}