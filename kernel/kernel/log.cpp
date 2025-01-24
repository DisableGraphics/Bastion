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