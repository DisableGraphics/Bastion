#include "string.h"

size_t strlen(const char* str) 
{
	size_t len;
	for(len = 0; *str; len++, str++);
	return len;
}