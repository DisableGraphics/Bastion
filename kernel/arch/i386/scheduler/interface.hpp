#include <stdint.h>

extern "C" void jump_fn(uint32_t **old_esp, 
		uint32_t *new_esp, 
		void (*fn)(), 
		uint8_t code_segment_selector, 
		uint8_t data_segment_selector, 
		uint8_t ring);