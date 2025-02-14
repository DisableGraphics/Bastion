#include <stddef.h>
#include <string.h>

int ccount(const char *str, char c) {
	int ret = 0;
	for(; *str; str++) if(*str == c) ret++;
	return ret;
}