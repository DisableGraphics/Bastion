#include <stdio.h>
#include <kernel/kernel/log.hpp>
#include <stdarg.h>

const char *loglevel_to_str(log::LOG_LEVEL level) {
	switch(level) {
		case log::INFO:
			return "INFO";
		case log::WARN:
			return "WARN";
		case log::ERROR:
			return "ERROR";
	}
	return "UNKNOWN LEVEL (POSSIBLE CORRUPTION)";
}

void log::log(LOG_LEVEL level, const char *__restrict format, ...) {
	serial_printf("[%s]: ", loglevel_to_str(level));
	va_list args;
	va_start(args, format);
	serial_printf(format, args);
	va_end(args);
	serial_printf("\n");
}