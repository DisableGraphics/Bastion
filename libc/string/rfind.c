#include <string.h>
#include <stdio.h>

const char* rfind(const char* haystack, char needle) {
	int start = strlen(haystack);
	const char* ptr = haystack + start;
	for(int i = start; i >= 0; i--, ptr--) {
		if(*ptr == needle) return ptr;
	}
	return NULL;
}