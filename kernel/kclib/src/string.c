#include <string.h>

size_t strlen(const char* str) 
{
	size_t len;
	for(len = 0; *str; len++, str++);
	return len;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    unsigned char * s1c = (unsigned char *)s1;
    unsigned char * s2c = (unsigned char *)s2;
    int ret = 0;
    for(size_t i = 0; i < n; i++) {
        if(s1c[i] != s2c[i]) {
            ret = s1c[i] - s2c[i];
            break;
        }
    }
    return ret;
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