#include <string.h>

size_t strlen(const char* str) 
{
	size_t len;
	for(len = 0; *str; len++, str++);
	return len;
}

int memcmp(const void* aptr, const void* bptr, size_t size) {
	const unsigned char* a = (const unsigned char*) aptr;
	const unsigned char* b = (const unsigned char*) bptr;
	for (size_t i = 0; i < size; i++) {
		if (a[i] < b[i])
			return -1;
		else if (b[i] < a[i])
			return 1;
	}
	return 0;
}

void* memcpy(void* __restrict dest, const void* __restrict src, size_t n) {
    unsigned char * destc = (unsigned char *)dest;
    unsigned char * srcc = (unsigned char *)src;
    for(size_t i = 0; i < n; i++) {
        destc[i] = srcc[i];
    }
    return dest;
}

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}