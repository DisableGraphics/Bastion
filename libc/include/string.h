#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
// memset but for volatile regions of memory
// Obviously it's not in the standard
volatile void* vmemset(volatile void*, int, size_t);
size_t strlen(const char*);
char *strcpy(char* __restrict, const char *__restrict);

const char* rfind(const char* haystack, char needle);
int strcasecmp(const char* a, const char* b);
int strcmp(const char* a, const char* b);
const char* strstr(const char* X, const char* Y);

#ifdef __cplusplus
}
#endif

#endif
