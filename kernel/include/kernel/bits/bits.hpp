#include <stdint.h>

namespace bits {
	[[gnu::no_caller_saved_registers]]
	uint8_t popcount(uint32_t i);
	[[gnu::no_caller_saved_registers]]
	uint32_t bcd2normal(uint32_t bcd);
};