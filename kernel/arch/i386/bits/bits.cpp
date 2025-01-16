#include <kernel/bits/bits.hpp>

[[gnu::no_caller_saved_registers]]
uint8_t bits::popcount(uint32_t i) {
	i = i - ((i >> 1) & 0x55555555);        // add pairs of bits
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);  // quads
	i = (i + (i >> 4)) & 0x0F0F0F0F;        // groups of 8
	i *= 0x01010101;                        // horizontal sum of bytes
	return  i >> 24;  
}

[[gnu::no_caller_saved_registers]]
uint32_t bits::bcd2normal(uint32_t bcd) {
	return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xf);
}