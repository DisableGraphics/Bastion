#pragma once
#include <stdio.h>
#include <time.h>
#include <kernel/kernel/const.hpp>

enum LOG_LEVEL {
	INFO,
	WARN,
	ERROR,
};

constexpr const char *loglevel_to_str(LOG_LEVEL level) {
	switch(level) {
		case INFO:
			return "INFO";
		case WARN:
			return "WARN";
		case ERROR:
			return "ERROR";
	}
	return "UNKNOWN LEVEL (POSSIBLE CORRUPTION)";
};

void set_log_printf(int (*printf_fn)(const char *__restrict format, ...));

extern int (*printf_fn)(const char *__restrict format, ...);

#define log(log_level, ...)  { printf_fn("[%s]: (@%s:%d %ds) ", loglevel_to_str(log_level), __FILE_NAME__, __LINE__, seconds_since_boot()); printf_fn(__VA_ARGS__); printf_fn("\n"); }

