#pragma once
#include <stdio.h>

enum LOG_LEVEL {
	INFO,
	WARN,
	ERROR,
};

const char *loglevel_to_str(LOG_LEVEL);

//void log(LOG_LEVEL, const char* __restrict, ...);
#define log(log_level, ...)  { serial_printf("[%s]: ", loglevel_to_str(log_level)); serial_printf(__VA_ARGS__); serial_printf("\n"); }

