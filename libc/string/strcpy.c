#include "string.h"

char *strcpy(char* __restrict dst, const char *__restrict src) {
	return memcpy(dst, src, strlen(src));
}