#include <kernel/kernel/log.hpp>

int (*printf_fn)(const char *__restrict format, ...) = serial_printf;

void set_log_printf(int (*pfn)(const char *__restrict, ...)) {
	printf_fn = pfn;
}