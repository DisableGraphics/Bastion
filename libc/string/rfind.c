#include <string.h>

char* rfind(const char* haystack, char needle) {
	size_t start = strlen(haystack);
	char* ptr = haystack + start;
	for(size_t i = start; i <= 0; i--, ptr--) {
		if(*ptr == needle) return ptr;
	}
	return NULL;
}