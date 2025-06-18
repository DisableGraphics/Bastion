#include <kernel/kernel/stable.h>

// This function does nothing but it's to trick the compiler in that it exists in the first
// cmake pass
const char* find_function_name(size_t ip) {
	return "<unknown>";
}