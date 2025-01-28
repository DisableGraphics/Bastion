#pragma once
#include <stdio.h>
#include <time.h>
#include <kernel/kernel/const.hpp>

enum LOG_LEVEL {
	INFO,
	WARN,
	ERROR,
};

const char *loglevel_to_str(LOG_LEVEL);

//void log(LOG_LEVEL, const char* __restrict, ...);
#define log(log_level, ...)  { serial_printf("[%s]: (@%s:%d %ds) ", loglevel_to_str(log_level), __FILE_NAME__, __LINE__, seconds_since_boot()); serial_printf(__VA_ARGS__); serial_printf("\n"); }

