#include <kernel/kernel/log.hpp>

const char *loglevel_to_str(LOG_LEVEL level) {
	switch(level) {
		case INFO:
			return "INFO";
		case WARN:
			return "WARN";
		case ERROR:
			return "ERROR";
	}
	return "UNKNOWN LEVEL (POSSIBLE CORRUPTION)";
}

int (*printf_fn)(const char *__restrict format, ...) = serial_printf;

void set_log_printf(int (*pfn)(const char *__restrict, ...)) {
	printf_fn = pfn;
}