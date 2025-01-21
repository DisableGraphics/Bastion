#pragma once

namespace log {
	enum LOG_LEVEL {
		INFO,
		WARN,
		ERROR,
	};

	void log(LOG_LEVEL, const char* __restrict, ...);
}
