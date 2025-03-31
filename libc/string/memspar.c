#include <string.h>
#include <stdint.h>

void* memspar(void* bufptr, uint32_t value, size_t size) {
	uint32_t* buf = (uint32_t*) bufptr;
	const size_t size_end = size / sizeof(uint32_t);
	for (size_t i = 0; i < size_end; i ++)
		buf[i] = value;
	return bufptr;
}