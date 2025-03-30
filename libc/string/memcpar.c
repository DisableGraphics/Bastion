#include <stdint.h>
#include <string.h>

void* memcpar(void* restrict dstptr, const void* restrict srcptr, size_t size) {
	uint64_t* dst = (uint64_t*) dstptr;
	const uint64_t* src = (const uint64_t*) srcptr;
	const size_t size_f = size>>3;
	for (size_t i = 0; i < size_f; i++)
		dst[i] = src[i];
	return dstptr;
}
